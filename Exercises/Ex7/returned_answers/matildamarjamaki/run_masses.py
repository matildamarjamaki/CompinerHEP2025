with open("br.sm2", "r") as f:
    lines = [line.strip() for line in f if line.strip() and not line.startswith("_")]

# Fix header parsing: combine "GAM GAM" → "GAM_GAM" and "Z GAM" → "Z_GAM"
header = lines[0].replace("GAM GAM", "GAM_GAM").replace("Z GAM", "Z_GAM").split()

if "WIDTH" not in header:
    print("Error: 'WIDTH' column not found in br.sm2.")
    exit()

width_index = header.index("WIDTH")  # Locate WIDTH column

# Open output file
with open("mass_width.txt", "w") as f_out:
    f_out.write("# Higgs Mass (GeV)   Width (GeV)\n")

    for line in lines[1:]:  # Skip header
        values = line.split()

        if len(values) < len(header):
            print(f"⚠ Warning: Line has only {len(values)} columns instead of {len(header)}: {line}")
            width = "N/A"  # Assign default value
        else:
            width = values[width_index]

        mass = values[0]  # First column is mass

        f_out.write(f"{mass} {width}\n")
        print(f"✔ Mass: {mass} GeV, Width: {width} GeV")
