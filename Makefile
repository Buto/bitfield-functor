
# builds UT for bitfields

# by 'gold' I mean "The output of a known good UT run."
GOLD_UT_RESULTS := ./ut_ref_output/control_board_gpio_reg23_ut_output.txt
UT_RESULTS  	:= ./control_board_gpio_reg23_ut_output.txt

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
control_board_gpio_reg23.compare_ut_gold:
	$(call compare_ut_gold,$(subst .compare_ut_gold,,$@))

.PHONY:	clean
clean:
	rm -f *.o *.exe *.stackdump *.core

ut_control_board_gpio_reg23.exe: ut_control_board_gpio_reg23.cpp control_board_gpio_reg23.h
	g++ -std=c++17 -Wall ut_control_board_gpio_reg23.cpp -o ut_control_board_gpio_reg23.exe




.PHONY:	control_board_gpio_reg23.gen_ut_ref_file
control_board_gpio_reg23.gen_ut_ref_file:
	mkdir -p ut_ref_output
	./ut_control_board_gpio_reg23.exe >  $(GOLD_UT_RESULTS)

.PHONY:	control_board_gpio_reg23.run_ut
control_board_gpio_reg23.run_ut:
	@./ut_$(subst .run_ut,,$@ ).exe >  $(UT_RESULTS)





.PHONY:	bitfield_all
bitfield_all:    ut_control_board_gpio_reg23.exe   control_board_gpio_reg23.run_ut  control_board_gpio_reg23.compare_ut_gold

.PHONY:	all
all:    bitfield_all








