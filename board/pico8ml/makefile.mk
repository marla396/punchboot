ifdef CONFIG_BOARD_PICO8ML

PB_BOARD_NAME = pico8ml
PB_PLAT_NAME  = imx8m
PB_ENTRY      = 0x7E1000

BOARD_C_SRCS += board/pico8ml/board.c

CFLAGS += -I board/pico8ml/include
endif
