import matplotlib.pyplot as plt 
import seaborn as sns 
import pandas as pd
import numpy as np

if __name__ == "__main__":
    sns.set(font_scale=1.75)
    df = pd.read_csv('benchmark_results.csv')
    plt.hist(df[df['Type'] == "Hit"].tail(1000)['Time'],np.linspace(0,1500,100),label='Hit')
    plt.hist(df[df['Type'] == "Miss"].tail(1000)['Time'],np.linspace(0,1500,100),label='Miss')
    plt.legend()
    plt.title('Latency Histogram of Cache Hits and Misses')
    plt.show()
