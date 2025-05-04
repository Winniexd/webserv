#COMPILER
NAME = webserv
CXX=c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -g

#FILES
INC = -Iinc/
FILES = main.cpp Server.cpp
SRCS = $(addprefix src/, $(FILES))
OBJ = $(SRCS:.cpp=.o)

#ANSI COLOR CODES
BLUE = \e[0;34m
GREEN = \e[0;32m
RESET = \e[0m

#PERCENTAGE BAR
COUNT = 0
TOTAL = $(words $(SRCS))

all: $(NAME)

$(NAME): $(OBJ)
	@$(CXX) $(CFLAGS) $(OBJ) $(INC) -o $(NAME)
	@printf "$(GREEN)\nCompilation complete!\n$(RESET)"

%.o: %.cpp
	@echo -n "\033[2K\rCompiling $< ... "
	@$(CXX) $(CFLAGS) $(INC) -c $< -o $@
	@echo "$(BLUE)done$(RESET)"
	$(eval COUNT := $(shell echo $$(( $(COUNT) + 1 ))))
	@BAR=$$(printf "%0.s#" $$(seq 1 $(COUNT))); \
	N=$$(( $(TOTAL) - $(COUNT) )); \
	SPACES=""; \
	if [ $$N -gt 0 ]; then SPACES=$$(printf "%0.s " $$(seq 1 $$N)); fi; \
	PERCENT=$$(( 100 * $(COUNT) / $(TOTAL) )); \
	printf "[\033[1;32m$$BAR$$SPACES\033[0m] $$PERCENT%%"

clean:
	@rm -rf $(OBJ)

fclean:
	@make clean
	@rm -rf $(NAME)

re: 
	@make fclean
	@make all

.PHONY: all clean fclean re