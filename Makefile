
# builds UT for bitfields

# by 'gold' I mean "The output of a known good UT run."
GOLD_UT_RESULTS := ./ut_ref_output/bitfield_ut_output.txt
UT_RESULTS  	:= ./bitfield_ut_output.txt

$(subst .run_ut,,$@)

#  The compare_ut_gold function compares a file
#  containing 'known good' UT results with a file
#  containing the outcome of the most recent UT run.
#
#  If these two files miscompare then we've encountered a UT failure
define compare_ut_gold =
	@# we cannot automatically verify that the UT passed without
	@# a file containing the output from an known-good UT run
	@#
	@# if a file containing known-good UT results doesn't exist
	@if [ ! -f ./ut_ref_output/$(1)_ut_output.txt ]; then             			\
	   echo "ERROR: File ./ut_ref_output/$(1)_ut_output.txt does not exist.";               \
	   echo "This Makefile will be unable to automatically determine if  ";                 \
	   echo "the '$(1)' passed its UT until this file is recreated.";             		\
	   echo " ";                  								\
	   echo "Recreate this file by issuing 'make $(1).gen_ut_ref_file'."; 			\
	   echo "Caution: verify that the $(1) UT passes before recreating this file."; 	\
	   exit 1;   \
	fi;          \
	echo " "; `#if the results of this UT run matches a 'known good' UT run` \
	cmp -s ./ut_ref_output/$(1)_ut_output.txt ./$(1)_ut_output.txt;   \
	RETVAL=$$?;                     \
	if [ $$RETVAL -eq 0 ]; then 	\
	    echo "$(1) UT passed";   	\
	else                     	\
	    cat ./$(1)_ut_output.txt;	\
	    echo "$(1) UT FAILED!";   	\
	fi
endef

# compare the results of a known-good UT run with outcome of the most recent UT run.
bitfield.compare_ut_gold:
	$(call compare_ut_gold,$(subst .compare_ut_gold,,$@))

.PHONY:	clean
clean:
	rm -f *.o *.exe *.stackdump *.core

ut_bitfield.exe: ut_bitfield.cpp bitfield.h
	g++ -std=c++17 ut_bitfield.cpp -o ut_bitfield.exe




.PHONY:	bitfield.gen_ut_ref_file
bitfield.gen_ut_ref_file:
	mkdir -p ut_ref_output
	./ut_bitfield.exe >  $(GOLD_UT_RESULTS)

.PHONY:	bitfield.run_ut
bitfield.run_ut:
	@./ut_$(subst .run_ut,,$@ ).exe >  $(UT_RESULTS)





.PHONY:	bitfield_all
bitfield_all:    ut_bitfield.exe   bitfield.run_ut  bitfield.compare_ut_gold

.PHONY:	all
all:    bitfield_all








