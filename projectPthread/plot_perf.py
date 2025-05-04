import pandas as pd
import matplotlib.pyplot as plt

# CSV betöltése
df = pd.read_csv("performance.csv")

# Ábrázolási stílus beállítása
plt.style.use("seaborn-v0_8-darkgrid")  # vagy 'ggplot', 'seaborn-darkgrid', stb.

# 1. Ábra: Futási idők
plt.figure(figsize=(10, 6))
plt.plot(df["Threads"], df["SequentialTime"], marker='o', linestyle='--', label="Szekvenciális idő (állandó)")
plt.plot(df["Threads"], df["ParallelTime"], marker='o', label="Párhuzamos idő")
plt.title("Futási idők különböző szálak esetén")
plt.xlabel("Szálak száma")
plt.ylabel("Futási idő (másodperc)")
plt.xticks(df["Threads"])
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("plot_futasi_idok.png")
plt.show()

# 2. Ábra: Speedup
plt.figure(figsize=(10, 6))
plt.plot(df["Threads"], df["Speedup"], marker='o', color='green', label="Speedup")
plt.plot(df["Threads"], df["Threads"], linestyle='--', color='gray', label="Ideális speedup")
plt.title("Speedup a szálak számának függvényében")
plt.xlabel("Szálak száma")
plt.ylabel("Speedup")
plt.xticks(df["Threads"])
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("plot_speedup.png")
plt.show()

# 3. Ábra: Hatékonyság
plt.figure(figsize=(10, 6))
plt.plot(df["Threads"], df["Efficiency"], marker='s', color='purple', label="Hatékonyság (%)")
plt.title("Hatékonyság a szálak számának függvényében")
plt.xlabel("Szálak száma")
plt.ylabel("Hatékonyság (%)")
plt.xticks(df["Threads"])
plt.ylim(0, 110)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("plot_hatekonysag.png")
plt.show()
