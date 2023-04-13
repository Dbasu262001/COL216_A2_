all: sample _79stage

sample: sample.cpp MIPS_Processor.hpp
	g++ sample.cpp MIPS_Processor.hpp -o sample
_79stage: 79stage.cpp MIPS_Processor.hpp
	g++ 79stage.cpp MIPS_Processor.hpp -o stage79
	g++ 79stage_bypass.cpp MIPS_Processor.hpp -o stage79_bypass
clean:
	rm sample stage79 stage79_bypass