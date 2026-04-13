# CSEE4840-Final-Project
Wavetable synthesizer for CSEE4840 final project. FPGA-specific files live in the 
hw directory, while all HPS/Linux code lives in the sw directory

# Design Doc
[Design Doc Google Drive Link ](https://docs.google.com/document/d/1s_W6IeTHjc9uK8sTTZ2ynrz7b73vRMTMJYI5ecwT3QM/edit?usp=sharing)


# Resources

- [audio core for de1-soc manual] (https://people.ece.cornell.edu/land/courses/ece5760/DE1_SOC/Audio_core.pdf)
- [audio video config core](https://ftp.intel.com/Public/Pub/fpgaup/pub/Intel_Material/18.1/University_Program_IP_Cores/Audio_Video/Audio_and_Video_Config.pdf)
- [cyclone v processor technical manual] - page 163 HPS_FPGA Bridges Address Map and Register Defintions (https://www.intel.com/programmable/technical-pdfs/683126.pdf)
  - [memory addresses for different busses](https://www.intel.com/content/www/us/en/programmable/hps/cyclone-v/hps.html)


## Tutorials


## Verilog
- [hdl bits](https://hdlbits.01xz.net/wiki/Main_Page) : great website for verilog excersises
- [verilator tutorial](https://itsembedded.com/dhd/verilator_1/) : verilator is the software we used in lab 1 for writing testbenches


### Fpga labs
- [Fpga Academy](https://fpgacademy.org/courses.html): Lots of helpful tutorials for working with our board
  - [Making a piano tutorial](https://github.com/fpgacademy/Lab_Exercises_Embedded_Systems/releases/download/v1.1/lab8.pdf)
  - [Making an Oscilliscope](https://github.com/fpgacademy/Lab_Exercises_Embedded_Systems/releases/download/v1.1/lab9.pdf)
  
## Similar Projects 
- [Synthesizer project from our class from 2019](https://www.cs.columbia.edu/~sedwards/classes/2019/4840-spring/reports/LoopStation.pdf)
- [Cornell Embedded Systems class final project ideas (audio signal processing section)](https://people.ece.cornell.edu/land/courses/ece5760/FinalProjects/ProjectIdeas.htm)

## Textbooks
- [System Verilog Textbook](./resources/system-verilog-textbook.pdf)
- [The Art of Digital Audio Textbook](https://velserver.no-ip.org/TVD/ebooks/The%20Art%20of%20Digital%20Audio.pdf)
