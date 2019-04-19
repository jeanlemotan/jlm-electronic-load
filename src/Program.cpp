#include "Program.h"
#include <vector>
#include <array>
#include "Measurement.h"
#include <Arduino.h>

extern Measurement s_measurement;

struct Step
{
	enum class Instruction
	{
		SetCurrent,
		LoadOn,
		LoadOff,
		Delay,
		Repeat,
		Call,
		Nop
	};

	Instruction instruction = Instruction::Nop;
	float targetCurrent = 0.f;
	size_t delayDuration = 0; //ms
	size_t repeatStep = 0;
	size_t repeatCount = 0;
	size_t programIndex = 0;
};

struct Program
{
	bool isValid = false;
	bool isRunning = false;
	std::vector<Step> steps;
};

static const size_t MAX_PROGRAMS = 10; //the 11 is the immediate one
static const size_t IMMEDIATE_PROGRAM = 10;
std::array<Program, 11> s_programs;

struct StackFrame
{
	size_t programIndex = 0;
	size_t step = 0;
	size_t subStep = 0;
	size_t stepStartTP = 0;
	size_t delayStartTP = 0;
	size_t repeatCount = 0;
};

std::vector<StackFrame> s_stackFrames;

void setStep(StackFrame& frame, size_t step)
{
	frame.step = step;
	frame.subStep = 0;
	frame.stepStartTP = millis();
}


void compileProgram(const char* code, Program& program)
{
	stopProgram();

	if (code == nullptr || *code == '\0')
	{
		printf("Empty program\n");
		return;
	}

	const char* s = code;

	bool ok = true;
	while (true)
	{
		while ((*s == ' ' || *s == '\t') && *s != '\0') s++;
		char opcode = *s++;
		if (opcode == '\n' || opcode == '\r' || opcode == '\0')
		{
			break;
		}
		if (*s == '\0')
		{
			printf("Unexpected end of program\n");
			ok = false;
			break;
		}

		switch (opcode)
		{
			case 'S':
			{
				Step step;
				step.instruction = Step::Instruction::SetCurrent;
				step.targetCurrent = atof(s);
				if (step.targetCurrent < 0.f || step.targetCurrent > Measurement::k_maxCurrent)
				{
					printf("Invalid set current %f. Min is 0 and max is %f\n", step.targetCurrent, Measurement::k_maxCurrent);
					ok = false;					
				}
				program.steps.push_back(step);
				printf(": Set current to %f A\n", step.targetCurrent);
			}
			break;
			case 'L':
			{
				Step step;
				step.instruction = atoi(s) == 1 ? Step::Instruction::LoadOn : Step::Instruction::LoadOff;
				program.steps.push_back(step);
				printf(": Load %s\n", step.instruction == Step::Instruction::LoadOn ? "on" : "off");
			}
			break;
			case 'D':
			{
				Step step;
				step.instruction = Step::Instruction::Delay;
				step.delayDuration = atoi(s);
				program.steps.push_back(step);
				printf(": Delay %d ms \n", step.delayDuration);
			}
			break;
			case 'R':
			{
				Step step;
				step.instruction = Step::Instruction::Repeat;
				step.repeatCount = atoi(s);
				program.steps.push_back(step);
				printf(": Repeat %d times \n", step.repeatCount);
			}
			break;
			case 'C':
			{
				Step step;
				step.instruction = Step::Instruction::Call;
				step.programIndex = atoi(s);
				if (step.programIndex >= MAX_PROGRAMS)
				{
					printf("Invalid program index %d. Max index is %d\n", int(step.programIndex), (int)MAX_PROGRAMS);
					ok = false;
					break;
				}				
				printf(": Call program %d ms \n", step.programIndex);
				program.steps.push_back(step);
			}
			break;
		}

		s = strchr(s, ' ');
		if (!s)
		{
			break;
		}
	}

	program.isValid = ok;
}

void compileProgram(const char* code)
{
	stopProgram();

	if (code == nullptr || *code == '\0')
	{
		printf("Empty program\n");
		return;
	}

	int programIndex = -1;

	if (*code >= '0' && *code <= '9')
	{
		programIndex = atoi(code);
		code = strchr(code, ' ');
		if (programIndex >= MAX_PROGRAMS)
		{
			printf("Invalid program index %d. Max index is %d\n", int(programIndex), (int)IMMEDIATE_PROGRAM);
			return;
		}
	}

	Program program;
	compileProgram(code, program);

	if (!program.isValid)
	{
		return;
	}

	if (programIndex >= 0)
	{
		s_programs[programIndex] = program;
	}
	else
	{
		s_programs[IMMEDIATE_PROGRAM] = program;
		stopProgram();
		runProgram(IMMEDIATE_PROGRAM);
	}
}

void runProgram(size_t index)
{
	if (index >= s_programs.size())
	{
		printf("Invalid program index %d. Max index is %d\n", int(index), (int)s_programs.size());
		return;
	}
	Program& program = s_programs[index];
	if (!program.isValid)
	{
		printf("Program %d is invalid\n", int(index));
		return;
	}

	stopProgram();

	StackFrame frame;
	frame.programIndex = index;
	program.isRunning = true;
	setStep(frame, 0);
	s_stackFrames.push_back(frame);
}

void stopProgram()
{
	s_stackFrames.clear();
	s_measurement.setLoadEnabled(false);
}

void popProgram()
{
	if (s_stackFrames.empty())
	{
		ESP_LOGE("Program", "Pop underflow");
		stopProgram();
		return;
	}
	s_stackFrames.pop_back();

	if (s_stackFrames.empty())
	{
		printf("Program ended\n");
		stopProgram();
		return;
	}
}

bool isRunningProgram()
{
	return !s_stackFrames.empty();
}


void updateProgram()
{
	if (s_stackFrames.empty())
	{
		return;
	}

	s_stackFrames.reserve(s_stackFrames.size() + 1); //to avoid allocations when calling
	StackFrame& frame = s_stackFrames.back();
	Program& program = s_programs[frame.programIndex];
	if (!program.isValid)
	{
		ESP_LOGE("Program", "Running an invalid program");
		stopProgram();
		return;
	}

	if (frame.step >= program.steps.size())
	{
		popProgram();
		return;
	}

	Step& step = program.steps[frame.step];
	switch (step.instruction)
	{
		case Step::Instruction::SetCurrent:
		{
			if (frame.subStep == 0)
			{	
				s_measurement.setTargetCurrent(step.targetCurrent);
				frame.subStep++;
			}
			if (s_measurement.isLoadSettled() || !s_measurement.isLoadEnabled() || millis() > frame.stepStartTP + 100)
			{
				setStep(frame, frame.step + 1);
			}
		}
		break;
		case Step::Instruction::LoadOn:
		{
			if (frame.subStep == 0)
			{
				s_measurement.setLoadEnabled(true);
			}
			if (s_measurement.isLoadSettled() || millis() > frame.stepStartTP + 100)
			{
				setStep(frame, frame.step + 1);
			}
		}
		break;
		case Step::Instruction::LoadOff:
		{
			s_measurement.setLoadEnabled(false);
			setStep(frame, frame.step + 1);
		}
		break;
		case Step::Instruction::Delay:
		{
			if (frame.subStep == 0)
			{
				frame.delayStartTP = millis();
				frame.subStep = 1;
			}
			else
			{
				if (millis() >= frame.delayStartTP + step.delayDuration)
				{
					setStep(frame, frame.step + 1);
				}
			}
		}
		break;
		case Step::Instruction::Repeat:
		{
			if (frame.repeatCount < step.repeatCount)
			{
				setStep(frame, step.repeatStep);
				frame.repeatCount++;
			}
			else
			{
				setStep(frame, frame.step + 1);
			}
		}
		break;
		case Step::Instruction::Call:
		{
			if (!s_programs[step.programIndex].isValid)
			{
				ESP_LOGE("Program", "Calling an invalid program");
				stopProgram();
				break;
			}
			setStep(frame, frame.step + 1);
			StackFrame frame;
			frame.programIndex = step.programIndex;
			s_stackFrames.push_back(frame);
		}
		break;
		default: 
			setStep(frame, frame.step + 1);
		break;
	}
}