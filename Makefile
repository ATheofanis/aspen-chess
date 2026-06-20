ifndef EXE
    EXE = aspen
    override NO_EXE_SET = true
    export NO_EXE_SET
endif

export EXE

ifndef EVALFILE
    EVALFILE = $(DEFAULT_NET).nnue
    override NO_EVALFILE_SET = true
endif

export EVALFILE