DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

MAKEDEP = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $<

$(OUTDIR)/%.o : %.c
$(OUTDIR)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(MAKEDEP) -o $@

$(DEPDIR): ; @mkdir -p $@
$(OUTDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
