# LC3-VM

Very basic LC-3 virtual machine to simulate the LC-3 instructions on any system

---

# Build

---

## Requirements

- GCC or any other C code compiler

The code is only one file and can easily be built using any C compiler

```bash
gcc -O3 -o main src/main.c
# Though not necessary, the -static flag can be used to create a statically linked binary
gcc -O3 -static -o main src/main.c
# The -g flag can be used to compile with debug flags in case someone wants to debug the binary
gcc -O3 -g -o main main.c

```

# Build (Docker)

---

## Requirements

- Docker (CLI or Desktop)

Though the code can easily be compiled on any system, there is also the option to use docker to build and run the image

```bash
docker build --build-arg obj_code=<your-object-code>.obj -t <image-name> .

```

# Running

---

## Compiled Locally

```bash
./main <your-object-code>.obj
```

## With Docker

```bash
# In order to run properly, the container needsd access to your your stdin (-i) and tty (-t)
docker run -it <image-name>
```

# Object codes

---

There is no current implementation to compile an object code for this virtual machine, two object codes are there in the object-code directory which can be used with the local binary or docker file
