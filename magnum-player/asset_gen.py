import sys

if len(sys.argv) < 4:
    print("Usage: asset_gen.py <input_file> <output_file> <cpp_buffer_name>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]
cpp_buffer_name = sys.argv[3]

with open(input_file, "rb") as f_in, open(output_file, "w") as f_out:
    data = f_in.read()
    f_out.write("// Auto-generated with asset_gen.py script\n\n")
    f_out.write("#pragma once\n")
    f_out.write("#include <cstdint>\n\n")
    f_out.write("namespace asset {\n\n")

    f_out.write(f"constexpr unsigned char {cpp_buffer_name}[] = {{")
    f_out.write(",".join(f"0x{byte:02x}" for byte in data))
    f_out.write("};\n")
    f_out.write(f"constexpr size_t {cpp_buffer_name}_Size = {len(data)};\n")

    f_out.write("\n}\n")
