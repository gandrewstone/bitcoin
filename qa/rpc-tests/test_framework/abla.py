#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin developers

"""The contents of this file mainly match what's on the C++ side in src/consensus/abla.*."""

from .cdefs import DEFAULT_CONSENSUS_BLOCK_SIZE, MAX_CONSENSUS_BLOCK_SIZE, ONE_MEGABYTE

B7 = 1 << 7
MAX_UINT64 = 2**64 - 1


def _muldiv(x, y, z):
    assert z != 0
    res = (x * y) // z
    assert res <= MAX_UINT64
    return res


# noinspection PyPep8Naming
class AblaConfig:
    __slots__ = ("epsilon0", "beta0", "gammaReciprocal", "zeta_xB7", "thetaReciprocal", "delta", "epsilonMax",
                 "betaMax")

    def __init__(self, maxBlockSize=DEFAULT_CONSENSUS_BLOCK_SIZE, fixedSize=False):
        self.epsilon0 = maxBlockSize // 2
        self.beta0 = maxBlockSize // 2
        self.gammaReciprocal = 37938
        self.zeta_xB7 = 192
        self.thetaReciprocal = 37938
        self.delta = 10
        if not fixedSize:
            self.setMax()
        else:
            self.epsilonMax = self.epsilon0
            self.betaMax = self.beta0

    def setMax(self):
        maxSafeBlocksizeLimit = MAX_UINT64 // self.zeta_xB7 * B7
        maxElasticBufferRatioNumerator = self.delta * ((self.zeta_xB7 - B7)
                                                           * self.thetaReciprocal // self.gammaReciprocal)
        maxElasticBufferRatioDenominator = (self.zeta_xB7 - B7) * self.thetaReciprocal // self.gammaReciprocal + B7
        self.epsilonMax = (maxSafeBlocksizeLimit
                           // (maxElasticBufferRatioNumerator + maxElasticBufferRatioDenominator)
                           * maxElasticBufferRatioDenominator)
        self.betaMax = maxSafeBlocksizeLimit - self.epsilonMax

    def __repr__(self):
        return f"<class 'AblaConfig' epsilon0={self.epsilon0} beta0={self.beta0}" \
               f" gammaReciprocal={self.gammaReciprocal} zeta_xB7={self.zeta_xB7}" \
               f" thetaReciprocal={self.thetaReciprocal} delta={self.delta} epsilonMax={self.epsilonMax}" \
               f" betaMax={self.betaMax}>"


# Regtest default
DEFAULT_ABLA_CONFIG = AblaConfig()


# noinspection PyPep8Naming
class AblaState:
    __slots__ = ("blockSize", "controlBlockSize", "elasticBufferSize")

    def __init__(self, config=DEFAULT_ABLA_CONFIG, blockSize=None, *, controlBlockSize=None, elasticBufferSize=None):
        self.blockSize = blockSize or 0
        self.controlBlockSize = config.epsilon0 if controlBlockSize is None else controlBlockSize
        self.elasticBufferSize = config.beta0 if elasticBufferSize is None else elasticBufferSize

    @classmethod
    def FromRpcDict(cls, d, config=DEFAULT_ABLA_CONFIG):
        if 'ablastate' in d:
            # Was a header, drill-down into the ablastate dict
            return cls.FromRpcDict(d['ablastate'], config=config)
        return cls(config=config, blockSize=d['blocksize'], controlBlockSize=d['epsilon'], elasticBufferSize=d['beta'])

    def GetBlockSizeLimit(self):
        return min(self.controlBlockSize + self.elasticBufferSize, MAX_CONSENSUS_BLOCK_SIZE)

    def GetNextBlockSizeLimit(self, config=DEFAULT_ABLA_CONFIG):
        return self.NextBlockState(config=config, nextBlockSize=0).GetBlockSizeLimit()

    def NextBlockState(self, nextBlockSize, config=DEFAULT_ABLA_CONFIG):
        ret = AblaState(config=config, blockSize=nextBlockSize)

        clampedBlockSize = min(self.blockSize, self.controlBlockSize + self.elasticBufferSize)
        amplifiedCurrentBlockSize = _muldiv(config.zeta_xB7, clampedBlockSize, B7)

        # control buffer function
        if amplifiedCurrentBlockSize > self.controlBlockSize:
            bytesToAdd = amplifiedCurrentBlockSize - self.controlBlockSize
            amplifiedBlockSizeLimit = _muldiv(config.zeta_xB7, self.controlBlockSize + self.elasticBufferSize, B7)
            bytesMax = amplifiedBlockSizeLimit - self.controlBlockSize
            scalingOffset = _muldiv(_muldiv(config.zeta_xB7, self.elasticBufferSize, B7), bytesToAdd, bytesMax)
            ret.controlBlockSize = self.controlBlockSize + (bytesToAdd - scalingOffset) // config.gammaReciprocal
        else:
            bytesToRemove = self.controlBlockSize - amplifiedCurrentBlockSize
            ret.controlBlockSize = self.controlBlockSize - bytesToRemove // config.gammaReciprocal
            ret.controlBlockSize = max(ret.controlBlockSize, config.epsilon0)

        # elastic buffer function
        bufferDecay = self.elasticBufferSize // config.thetaReciprocal
        if amplifiedCurrentBlockSize > self.controlBlockSize:
            bytesToAdd = (ret.controlBlockSize - self.controlBlockSize) * config.delta
            ret.elasticBufferSize = self.elasticBufferSize - bufferDecay + bytesToAdd
        else:
            ret.elasticBufferSize = self.elasticBufferSize - bufferDecay

        ret.elasticBufferSize = max(ret.elasticBufferSize, config.beta0)
        ret.controlBlockSize = min(ret.controlBlockSize, config.epsilonMax)
        ret.elasticBufferSize = min(ret.elasticBufferSize, config.betaMax)

        return ret

    def CalcLookaheadBlockSizeLimit(self, count, config=DEFAULT_ABLA_CONFIG):
        lookaheadState = self
        for i in range(count):
            maxSize = lookaheadState.GetNextBlockSizeLimit(config=config)
            lookaheadState = lookaheadState.NextBlockState(config=config, nextBlockSize=maxSize)
        return lookaheadState.GetBlockSizeLimit()

    def __repr__(self):
        return f"<class 'AblaState' blockSize={self.blockSize} controlBlockSize={self.controlBlockSize}" \
               f" elasticBufferSize={self.elasticBufferSize}>"

    def __str__(self):
        return repr(self)

    def as_tup(self):
        return self.blockSize, self.controlBlockSize, self.elasticBufferSize

    def __eq__(self, other):
        return self.as_tup() == other.as_tup()

    def __ne__(self, other):
        return self.as_tup() != other.as_tup()

    def __hash__(self):
        return hash(self.as_tup())


if __name__ == "__main__":
    # 2MB blocksize limit
    conf = AblaConfig(maxBlockSize=2*ONE_MEGABYTE)
    state = AblaState(blockSize=1129, config=conf)
    print(repr(conf))
    print(repr(state))
    assert state.CalcLookaheadBlockSizeLimit(2048, config=conf) == 2234156

    # 32MB blocksize limit, already-advanced state
    conf = DEFAULT_ABLA_CONFIG
    state = AblaState(blockSize=403438931, config=conf, controlBlockSize=93377312, elasticBufferSize=310061619)
    print(repr(conf))
    print(repr(state))
    assert state.CalcLookaheadBlockSizeLimit(500, config=conf) == 406127292
