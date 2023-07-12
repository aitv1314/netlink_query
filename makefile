
# 目标：要生成的目标文件
# 依赖：目标文件由哪些文件生成
# 命令：通过执行该命令由依赖文件生成目标

# 普通变量

# 自带变量
#  CC = gcc #arm-linux-gcc
#  CPPFLAGS : C预处理的选项 -I
#  CFLAGS:   C编译器的选项 -Wall -g -c
#  LDFLAGS :  链接器选项 -L  -l

# 自动变量
#  $@ 表示规则中的目标
#  $^ 表示规则中的所有条件, 组成一个列表, 以空格隔开, 如果这个列表中有重复的项则消除重复项
#  $< 表示规则中的第一个条件
#  $? 第一变化的依赖


ARCH = arm
CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = -ll
CFLAGS = -Wall

LIBS = 
# wildcard查找当前目录下指定类型的文件
SRC = $(wildcard *.cpp)

# SRC = $(OBJ:.o=.c) 一种替换方式
# 另一种替换方式
OBJ = $(patsubst %.cpp, %.o, $(SRC))

TARGET = main

all: $(TARGET)

#  "-@rm -f *.o" 减号的作用是当指令报错的时候依然执行，@的作用是不输出指令。
clean:
	-@rm -f $(OBJ) $(TARGET)

distclean: clean
	-@rm -f .o .s .d

$(TARGET):$(OBJ)
	$(CC) -o $@ $^	

# %为依赖条件
# 模式匹配规则，$@,$< 这样的变量，只能在规则中出现
%.o:%.c
	$(CC) -o $@ -c $< $(CPPFLAGS)

# 伪目标声明，防止文件夹有all clean等同名文件时产生歧义
.PHONY: all clean distclean
