target = ./bin/main
src = $(wildcard ./*.c)
head = $(src:%.c:%.h)
obj_path := ./bin/
obj := $(addprefix $(obj_path), $(src:%.c=%.o))

$(target) : $(obj)
	gcc $^ -o $@ -pthread

%.o : %.c $(head)
	gcc -c $< -o ./bin/$@

