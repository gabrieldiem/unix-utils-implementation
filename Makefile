CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=c11
CFLAGS += -Wvla
CPPFLAGS := -D_DEFAULT_SOURCE
LDFLAGS := -lrt

PROGS := ps find ls timeout infloop cp

all: $(PROGS)

# Add real time library during linking phase
%: %.o
	$(CC) -o $@ $^ $(LDFLAGS)

ps: ps.o
find: find.o
ls: ls.o
timeout: timeout.o
infloop: infloop.o
cp: cp.o

format: .clang-files .clang-format
	xargs -r clang-format -i <$<

clean:
	rm -f $(PROGS) *.o core vgcore.*

docker-build:
	docker build -t diem_challenges:latest .

docker-run:
	docker container run -it --rm --name=diem_challenges \
		-v $(shell pwd):/diem_challenges \
		--cap-add SYS_ADMIN \
		--security-opt apparmor:unconfined \
		diem_challenges:latest \
		bash 

docker-attach:
	docker exec -it diem_challenges bash

.PHONY: all clean format test