#!/bin/bash
for filename in ./tests/*; do
        riscv64-linux-gnu-objcopy "$filename" -O binary "$filename.bin"
	rm "$filename"
done
