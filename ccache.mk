USE_CCACHE?=1
ifneq ($(USE_CCACHE),0)
ifneq ($(shell command -v ccache),)
	CC := ccache $(CC)
endif
endif
