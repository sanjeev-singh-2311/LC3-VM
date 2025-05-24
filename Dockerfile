FROM gcc:15.1.0 as builder

WORKDIR lc3_vm

COPY src/* .

RUN gcc -static -O3 -o main main.c

FROM alpine:20250108 as runner

WORKDIR app

ARG obj_code

COPY $obj_code ./prog.obj
COPY --from=builder lc3_vm/main .

RUN chmod +x ./main

ENTRYPOINT ["./main"]
CMD ["prog.obj"]
