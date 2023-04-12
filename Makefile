all: sample _79stage

sample: sample.cpp MIPS_Processor.hpp
	g++ sample.cpp MIPS_Processor.hpp -o sample
_79stage: 79stage.cpp MIPS_Processor.hpp
	g++ 79stage.cpp MIPS_Processor.hpp -o stage79
clean:
	rm sample stage79