
# Avaetaan FeynHiggsin tulostiedosto
with open("feynhiggs_output.txt", "r") as file:
    lines = file.readlines()

# Tallennetaan massat ja kokonaisleveydet
masses = [100.000, 105.556, 111.111, 116.667, 122.222, 127.778, 133.333, 138.889, 144.444, 150.000]
widths = []

# Käydään rivit läpi ja etsitään "GammaTot"
for line in lines:
    if "GammaTot" in line:
        parts = line.split("=")  # Oletetaan muoto: "GammaTot = 4.07E-03"
        if len(parts) > 1:
            width = float(parts[1].strip())  # Poistetaan ylimääräiset merkit ja muunnetaan luvuksi
            widths.append(width)

# Varmistetaan, että arvojen määrä täsmää massojen kanssa
if len(widths) != len(masses):
    print("Virhe: massojen ja leveyksien lukumäärät eivät täsmää!")
else:
    # Kirjoitetaan uusi tiedosto
    with open("feynhiggs_widths3.txt", "w") as out_file:
        out_file.write("   MHSM          GG     GAM GAM     Z GAM         WW         ZZ       WIDTH\n")
        out_file.write("_______________________________________________________________________________\n")
        
        for i in range(len(masses)):
            out_file.write(f" {masses[i]:7.3f}     X.XXXXE-01 X.XXXXE-02 X.XXXXE-04 X.XXXXE-01 X.XXXXE-02 {widths[i]:.4E}\n")

    print("Tiedosto feynhiggs_widths.txt luotu onnistuneesti!")
