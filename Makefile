all: sample _79stage

sample: sample.cpp MIPS_Processor_79.hpp
	g++ sample.cpp MIPS_Processor_79.hpp -o sample
_79stage: 79stage.cpp MIPS_Processor_79.hpp
	g++ 79stage.cpp MIPS_Processor_79.hpp -o stage79
	g++ 79stage_bypass.cpp MIPS_Processor_79.hpp -o stage79_bypass
run_79stage:
	./stage79 input.asm
run_79stage_bypass:
	./stage79_bypass input.asm
clean:
	rm sample stage79 stage79_bypass