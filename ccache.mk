ifneq ($(shell command -v ccache),)
	CC := ccache $(CC)
endif
