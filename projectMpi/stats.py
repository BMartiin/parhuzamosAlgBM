import pandas as pd
import glob
import matplotlib.pyplot as plt

def main():
    files = glob.glob("stats_rank*.csv")

    if not files:
        print("Nincs statisztikai fájl (stats_rank*.csv) a mappában.")
        return

    dataframes = []
    for file in files:
        try:
            df = pd.read_csv(file)
            dataframes.append(df)
        except Exception as e:
            print(f"Hiba a fájl beolvasásakor: {file} – {e}")

    if not dataframes:
        print("Nem sikerült beolvasni egyetlen fájlt sem.")
        return

    df_all = pd.concat(dataframes, ignore_index=True)

    if 'rank' not in df_all.columns:
        print("Hiányzik a 'rank' oszlop a bemeneti fájlokból.")
        return

    df_all = df_all.sort_values("rank")
    df_all.to_csv("stats.csv", index=False)
    print("Összesített statisztika mentve: stats.csv")

    print("\n--- Összesített statisztika ---")
    print(df_all)

    # GRAFIKONOK
    ranks = df_all["rank"]
    visited = df_all["visited"]
    sent = df_all["sent"]
    received = df_all["received"]
    
    runtime_col = "runtime" if "runtime" in df_all.columns else "time"
    runtime = df_all[runtime_col]


    plt.figure(figsize=(12, 8))

    # 1. Feldolgozott URL-ek
    plt.subplot(2, 2, 1)
    plt.bar(ranks, visited, color='skyblue')
    plt.title("Feldolgozott URL-ek száma")
    plt.xlabel("Rank")
    plt.ylabel("Darabszám")

    # 2. Küldött vs Kapott linkek
    plt.subplot(2, 2, 2)
    plt.plot(ranks, sent, label="Küldött", marker='o')
    plt.plot(ranks, received, label="Kapott", marker='x')
    plt.title("Küldött vs Kapott linkek")
    plt.xlabel("Rank")
    plt.ylabel("Linkek száma")
    plt.legend()

    # 3. Futási idő
    plt.subplot(2, 2, 3)
    plt.bar(ranks, runtime, color='salmon')
    plt.title("Futási idő (mp)")
    plt.xlabel("Rank")
    plt.ylabel("Idő [s]")

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
