"""
Converted from data_analysis_questionnair.ipynb.
"""

# Questionnair Analysis

# Imports
import os
import re

import matplotlib.cm as cm
import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error, r2_score

from load_data import load_data_directory, load_data_file


# Load Data
households = pd.read_csv(
    "data/Questionnaires/Household Questionnaire (Responses) - Form Responses 1.csv"
)
indoor_air_quality_health = pd.read_csv(
    "data/Questionnaires/Indoor Air Quality & Health Check-In (Responses) - Form Responses 1.csv"
)

# Drop identifying columns if present in the files
households = households.drop(
    columns=["Enter Your Name Here\n", "Enter your College Email:"], errors="ignore"
)
indoor_air_quality_health = indoor_air_quality_health.drop(
    columns=["Enter Your Name:", "Enter Your College Email:"], errors="ignore"
)


# Make Plots
def _safe_filename(text, max_len=150):
    text = str(text).strip()
    text = re.sub(r'[<>:\\"/\\|?*\r\n\t]+', '_', text)
    text = re.sub(r'\s+', '_', text)
    text = re.sub(r'_+', '_', text).strip('._')
    if not text:
        text = 'unnamed_column'
    return text[:max_len]


def make_pi_plots(data=None, data_name=None):
    if data is None:
        raise ValueError("Data must be provided for plotting.")
    if data_name is None:
        raise ValueError("Data name must be provided for plot titles.")

    # Create plots directory if it doesn't exist
    output_dir = os.path.join("data", "Questionnaires", "Plots", data_name)
    os.makedirs(output_dir, exist_ok=True)

    # Loop through each column in the DataFrame
    for column in data.columns:
        # Skip timestamp column if it exists
        if str(column).strip().lower() == "timestamp":
            continue

        # Include missing values as a category and convert labels to string
        value_counts = data[column].fillna("Missing").astype(str).value_counts()
        if value_counts.empty:
            print(f"Skipped empty column: {data_name} - {column}")
            continue

        # Create pie chart
        plt.figure(figsize=(8, 8))
        plt.pie(value_counts, labels=value_counts.index, autopct="%1.1f%%", startangle=140)
        plt.title(f"{data_name} - {column}")
        plt.axis("equal")

        # Save plot with a Windows-safe file name
        column_safe = _safe_filename(column)
        save_path = os.path.join(output_dir, f"{column_safe}.png")
        plt.savefig(save_path, dpi=600)
        print(f"Saved pie chart for {data_name} - {column}")
        plt.show()
        plt.close()


if __name__ == "__main__":
    make_pi_plots(households, "Household_Questionnaire")
    make_pi_plots(indoor_air_quality_health, "Indoor_Air_Quality_Health_CheckIn")
