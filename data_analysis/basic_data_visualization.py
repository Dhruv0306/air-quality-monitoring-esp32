# Imports
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np
import os
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error, r2_score
from load_data import load_data_file, load_data_directory


# Load data for date : 31/12/2025
def basic_data_visualization():
    # Load data for date : 31/12/2025
    bedroom = load_data_file(
        "data\\Dhruv_Patel\\Bedroom\\ACRL_005_AU_PMS_CAPSTONE_2025-12-31.csv"
    )
    hall = load_data_file(
        "data\\Dhruv_Patel\\Hall\\ACRL_008_AU_PMS_CAPSTONE_2025-12-31.csv"
    )
    kitchen = load_data_file(
        "data\\Dhruv_Patel\\Kitchen\\ACRL_002_AU_PMS_CAPSTONE_2025-12-31.csv"
    )

    # Print sample data
    if bedroom is not None:
        print("Bedroom Data:")
        print(bedroom.head())
        # Drop columns 'Pressure', 'Gas', 'Altitude', 'pms_temp', 'pms_humidity', 'pms_formaldihyde', 'Latitude', 'Longitude', 'Altitude_GPS', 'Satellite'
        bedroom = bedroom.drop(
            columns=[
                "Pressure",
                "Gas",
                "Altitude",
                "pms_temp",
                "pms_humidity",
                "pms_formaldihyde",
                "Latitude",
                "Longitude",
                "Altitude_GPS",
                "Satellite",
            ],
            errors="ignore",
        )
        print("Bedroom Data after dropping columns:")
        print(bedroom.head())

    if hall is not None:
        print("Hall Data:")
        print(hall.head())
        # Drop columns 'Pressure', 'Gas', 'Altitude', 'pms_temp', 'pms_humidity', 'pms_formaldihyde', 'Latitude', 'Longitude', 'Altitude_GPS', 'Satellite'
        hall = hall.drop(
            columns=[
                "Pressure",
                "Gas",
                "Altitude",
                "pms_temp",
                "pms_humidity",
                "pms_formaldihyde",
                "Latitude",
                "Longitude",
                "Altitude_GPS",
                "Satellite",
            ],
            errors="ignore",
        )
        print("Hall Data after dropping columns:")
        print(hall.head())

    if kitchen is not None:
        print("Kitchen Data:")
        print(kitchen.head())
        # Drop columns 'Pressure', 'Gas', 'Altitude', 'pms_temp', 'pms_humidity', 'pms_formaldihyde', 'Latitude', 'Longitude', 'Altitude_GPS', 'Satellite'
        kitchen = kitchen.drop(
            columns=[
                "Pressure",
                "Gas",
                "Altitude",
                "pms_temp",
                "pms_humidity",
                "pms_formaldihyde",
                "Latitude",
                "Longitude",
                "Altitude_GPS",
                "Satellite",
            ],
            errors="ignore",
        )
        print("Kitchen Data after dropping columns:")
        print(kitchen.head())

    # Plot data (Same column in `Hall`, `Bedroom`, `Kitchen`) in a graph and save
    if bedroom is not None and hall is not None and kitchen is not None:
        bedroom["DateTime"] = pd.to_datetime(
            bedroom["DateTime"], format="%d-%m-%Y %H:%M:%S"
        )
        hall["DateTime"] = pd.to_datetime(hall["DateTime"], format="%d-%m-%Y %H:%M:%S")
        kitchen["DateTime"] = pd.to_datetime(
            kitchen["DateTime"], format="%d-%m-%Y %H:%M:%S"
        )

        data_columns = [col for col in bedroom.columns if col != "DateTime"]

        for column in data_columns:
            if column in bedroom.columns:
                plt.figure(figsize=(12, 6))
                plt.plot(hall["DateTime"], hall[column], label="Hall", alpha=0.7)
                plt.plot(bedroom["DateTime"], bedroom[column], label="Bedroom", alpha=0.7)
                plt.plot(kitchen["DateTime"], kitchen[column], label="Kitchen", alpha=0.7)

                plt.title(f"{column} - Temporal Data Comparison")
                plt.xlabel("Time")
                plt.ylabel(column)
                plt.legend()
                plt.grid(True, alpha=0.3)

                # Format x-axis to show only time
                plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
                plt.xticks(rotation=45)
                plt.tight_layout()

                os.makedirs("data\\Dhruv_Patel\\Plots", exist_ok=True)
                plt.savefig(
                    f"data\\Dhruv_Patel\\Plots\\{column}_temporal_comparison.png",
                    dpi=300,
                    bbox_inches="tight",
                )
                plt.close()

        print("All plots saved successfully!")


if __name__ == "__main__":
    basic_data_visualization()