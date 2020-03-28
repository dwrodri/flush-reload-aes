import matplotlib.pyplot as plt 
import seaborn as sns 
import pandas as pd
import numpy as np

if __name__ == "__main__":
    sns.set(font_scale=1.75)
    df = pd.read_csv('bench.csv')
    plt.hist(df[df['Type'] == "Hit"]['Time'],np.linspace(0,200,100),label='Hit')
    plt.hist(df[df['Type'] == "Miss"]['Time'],np.linspace(0,200,100),label='Miss')
    plt.legend()
    plt.title('Latency Histogram of Cache Hits and Misses')
    plt.show()
