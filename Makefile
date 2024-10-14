#
# Makefile
# Computer Networking Programing Assignments
#
#  Created by Phillip Romig on 4/3/12.
#  Copyright 2012 Colorado School of Mines. All rights reserved.
#
#
# Set your username here. E.G. USERNAME=promig3
USERNAME=pjwetherell

CXX = g++
LD = g++
CXXFLAGS = -g -std=c++17
LDFLAGS = -g 

#
# You should be able to add object files here without changing anything else
#
TARGET = web_server
OBJ_FILES = ${TARGET}.o
INC_FILES = ${TARGET}.h


${TARGET}: ${OBJ_FILES}
	${LD} ${LDFLAGS} ${OBJ_FILES} -o $@

%.o : %.cc ${INC_FILES}
	${CXX} -c ${CXXFLAGS} -o $@ $<

#
# Please remember not to submit objects or binarys.
#
clean:
	rm -f core ${TARGET} ${OBJ_FILES}

#
# This might work to create the submission tarball in the formal I asked for.
#
submit:
	@if [ -z "${USERNAME}" ]; then \
		echo "USERNAME variable is not set."; \
	else \
		echo "USERNAME variable is set to ${USERNAME}."; \
		rm -f core project1 ${OBJ_FILES}; \
		mkdir ${USERNAME}; \
		cp Makefile README.md *.h *.cpp ${USERNAME}; \
		tar zcf ${USERNAME}.tgz ${USERNAME}; \
		echo "Don't forget to upload ${USERNAME}.tgz to Canvas."; \
	fi
