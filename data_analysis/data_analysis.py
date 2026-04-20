"""
Data analysis for indoor PM2.5 across Kitchen, Hall, and Bedroom.

This script is a direct conversion of the original notebook. It loads sensor
data, computes summary statistics, and saves plots under
`data/Dhruv_Patel/Plots`.
"""

# --- Markdown cell 0 ---
# # This file contains data analyasis performed over full dataset


# --- Markdown cell 1 ---
# ## Imports


# --- Code cell 2 ---
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.colors import Normalize
import matplotlib.cm as cm
import numpy as np
import seaborn as sns
import os
import sys
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error, r2_score
from load_data import load_data_file, load_data_directory

# Configure UTF-8 encoding for console output to handle special characters like µ
if sys.platform == "win32":
    import io

    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

# --- Colours ---
kitchen_colour = "#E41A1C"  # red
hall_colour = "#4DAF4A"  # green
bedroom_colour = "#377EB8"  # blue
shaded_area_colour = "#FF7F00"  # orange
who_guideline_colour = "#984EA3"  # purple
india_guideline_colour = "#A65628"  # brown
outdoor_colour = "#999999"  # grey

# --- Markdown cell 3 ---
# ## Data Directory Paths


# --- Code cell 4 ---
# Base directories for input data and output plots.
DATA_DIR = "data/Dhruv_Patel/"
BEDROOM_DIR = f"{DATA_DIR}/Bedroom"
HALL_DIR = f"{DATA_DIR}/Hall"
KITCHEN_DIR = f"{DATA_DIR}/Kitchen"
PLOT_DIR = f"{DATA_DIR}/Plots"
OUTDOOR_DATA = f"{DATA_DIR}/outdoor_data.csv"


# --- Markdown cell 5 ---
# ## Load data for all 3 rooms


# --- Code cell 6 ---
# ============================================================================
# Remove None Values and Convert Data to 1 Hour Average
# ============================================================================
def remove_none_and_average(data, name):
    print(f"{name} shape before removing none values: {data.shape}")
    # Remove None values from all the coloums
    data = data.dropna()
    print(f"{name} shape after removing none values: {data.shape}")

    print(f"{name} shape before 1 hour average: {data.shape}")
    # Convert to datetime if not already
    data["DateTime"] = pd.to_datetime(data["DateTime"])

    # Set Time as index
    data.set_index("DateTime", inplace=True)

    # Resample to 30 minute averages
    data = data.resample("30min").mean()

    print(f"{name} shape after 30 minute average: {data.shape}")

    return data


def main():
    """
    Main execution function that loads data and runs all analyses.
    """
    kitchen_df = get_data(KITCHEN_DIR)
    hall_df = get_data(HALL_DIR)
    bedroom_df = get_data(BEDROOM_DIR)
    outdoor_df = pd.read_csv(OUTDOOR_DATA)
    outdoor_df.columns = outdoor_df.columns.str.strip()
    outdoor_df = outdoor_df.rename(columns={"date": "DateTime"})
    outdoor_df["DateTime"] = pd.to_datetime(outdoor_df["DateTime"])
    outdoor_df = outdoor_df.set_index("DateTime")[["pm25"]]
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")
    if outdoor_df is None:
        raise ValueError("outdoor_df is None")
    # Convert data to 1 hour average for outdoor data to match indoor data frequency
    kitchen_df = remove_none_and_average(kitchen_df, "Kitchen")
    hall_df = remove_none_and_average(hall_df, "Hall")
    bedroom_df = remove_none_and_average(bedroom_df, "Bedroom")
    min_date = min(
        kitchen_df.index.min(),
        hall_df.index.min(),
        bedroom_df.index.min(),
    )
    max_date = max(
        kitchen_df.index.max(),
        hall_df.index.max(),
        bedroom_df.index.max(),
    )
    # Convert min_date and max_date to datetime if they are not already
    # Convert datetime fromate to only date format (YYYY-MM-DD) to match outdoor data date format
    min_date = pd.to_datetime(min_date).date()
    max_date = pd.to_datetime(max_date).date()
    print(
        f"Filtering outdoor data to match indoor data date range: {min_date} to {max_date}"
    )
    # Convert outdoor_df index to date format (YYYY-MM-DD) to match min_date and max_date format
    outdoor_df.index = pd.to_datetime(outdoor_df.index).date
    outdoor_df = outdoor_df[
        (outdoor_df.index >= min_date) & (outdoor_df.index <= max_date)
    ]

    print("=" * 80)
    print("Running Multi-Day Time Series Analysis...")
    print("=" * 80)
    multi_day_time_series(
        kitchen_df=kitchen_df,
        hall_df=hall_df,
        bedroom_df=bedroom_df,
        outdoor_df=outdoor_df,
    )

    print("\n" + "=" * 80)
    print("Running Daily Average Trend Plot Analysis...")
    print("=" * 80)
    daily_average_trend_plot(
        kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df
    )

    print("\n" + "=" * 80)
    print("Running Daily Boxplot Analysis...")
    print("=" * 80)
    bedroom_df_copy = bedroom_df.copy()
    hall_df_copy = hall_df.copy()
    kitchen_df_copy = kitchen_df.copy()
    create_daily_boxplot(
        kitchen_df=kitchen_df_copy, hall_df=hall_df_copy, bedroom_df=bedroom_df_copy
    )

    print("\n" + "=" * 80)
    print("Running Diurnal Pattern Analysis...")
    print("=" * 80)
    kitchen_base_avg, hall_base_avg, bedroom_base_avg = diurnal_pattern(
        kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df
    )

    print("\n" + "=" * 80)
    print("Running Weekday vs Weekend Comparison Analysis...")
    print("=" * 80)
    weekdays_vs_weekends_comparison(
        kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df
    )

    print("\n" + "=" * 80)
    print("Running Percentage Time Above Guidelines Analysis...")
    print("=" * 80)
    percentage_time_above_guidelines(
        kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df
    )

    print("\n" + "=" * 80)
    print("Running Peak Event Analysis...")
    print("=" * 80)
    peak_event_analysis(
        kitchen_df=kitchen_df,
        hall_df=hall_df,
        bedroom_df=bedroom_df,
        kitchen_base_avg=kitchen_base_avg,
        hall_base_avg=hall_base_avg,
        bedroom_base_avg=bedroom_base_avg,
    )

    print("\n" + "=" * 80)
    print("Running Cooking-Related PM2.5 Contribution Analysis...")
    print("=" * 80)
    calculate_cooking_related_PM2_5_contribution_analysis(
        room_df=bedroom_df,
        room_name="Bedroom",
        room_base_avg=bedroom_base_avg,
        room_colour=bedroom_colour,
    )
    calculate_cooking_related_PM2_5_contribution_analysis(
        room_df=hall_df,
        room_name="Hall",
        room_base_avg=hall_base_avg,
        room_colour=hall_colour,
    )
    calculate_cooking_related_PM2_5_contribution_analysis(
        room_df=kitchen_df,
        room_name="Kitchen",
        room_base_avg=kitchen_base_avg,
        room_colour=kitchen_colour,
    )

    print("\n" + "=" * 80)
    print("Running Decay Rate After Cooking Analysis...")
    print("=" * 80)
    kitchen_decay_df = decay_rate_after_cooking(
        room_df=kitchen_df,
        room_colour=kitchen_colour,
        room_name="Kitchen",
        room_base_avg=kitchen_base_avg,
    )
    hall_decay_df = decay_rate_after_cooking(
        room_df=hall_df,
        room_colour=hall_colour,
        room_name="Hall",
        room_base_avg=hall_base_avg,
    )
    bedroom_decay_df = decay_rate_after_cooking(
        room_df=bedroom_df,
        room_colour=bedroom_colour,
        room_name="Bedroom",
        room_base_avg=bedroom_base_avg,
    )
    if not kitchen_decay_df.empty:
        print(
            f"Average Decay Rate After Cooking Events - Kitchen: {kitchen_decay_df['Decay Rate (µg/m³/hour)'].mean():.2f}"
        )
        print(
            f"Average Decay Duration After Cooking Events - Kitchen: {kitchen_decay_df['Decay Duration (hours)'].mean():.2f} hours"
        )
    if not hall_decay_df.empty:
        print(
            f"Average Decay Rate After Cooking Events - Hall: {hall_decay_df['Decay Rate (µg/m³/hour)'].mean():.2f}"
        )
        print(
            f"Average Decay Duration After Cooking Events - Hall: {hall_decay_df['Decay Duration (hours)'].mean():.2f} hours"
        )
    if not bedroom_decay_df.empty:
        print(
            f"Average Decay Rate After Cooking Events - Bedroom: {bedroom_decay_df['Decay Rate (µg/m³/hour)'].mean():.2f}"
        )
        print(
            f"Average Decay Duration After Cooking Events - Bedroom: {bedroom_decay_df['Decay Duration (hours)'].mean():.2f} hours"
        )

    print("\n" + "=" * 80)
    print("Running Spatial Gradient Analysis...")
    print("=" * 80)
    spatial_gradient_plot(kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df)

    print("\n" + "=" * 80)
    print("Running Particle Size Distribution Analysis...")
    print("=" * 80)
    particle_size_distribution(
        kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df
    )

    print("\n" + "=" * 80)
    print("Running Correlation Heatmap Analysis...")
    print("=" * 80)
    correlation_heatmap(kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df)
    print("\n" + "=" * 80)
    print("Running Frequency Analysis...")
    print("=" * 80)
    frequency_analysis(kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df)

    print("\n" + "=" * 80)
    print("24-Hour Average Analysis (WHO & NAAQS Guidelines Comparison)...")
    print("=" * 80)
    daily_average_guideline_comparison(
        kitchen_df=kitchen_df, hall_df=hall_df, bedroom_df=bedroom_df
    )


def get_data(roomPath=None):
    """
    Load and concatenate all sensor files from a room directory.

    Parameters
    ----------
    roomPath : str
        Directory path containing room data files.

    Returns
    -------
    pandas.DataFrame
        Concatenated dataframe for the room, or None if no data files exist.
    """
    if roomPath is None:
        raise ValueError("parameter roomPath is None")
    else:
        # `load_data_directory` returns a dict of {filename: dataframe}.
        data = load_data_directory(roomPath)
        dfs = []
        if data is not None:
            for file_name, df in data.items():
                if df is not None:
                    dfs.append(df)
        if dfs:
            return pd.concat(dfs, ignore_index=True)


# --- Markdown cell 7 ---
# ## Multi-Day Time Series (Foundation Plot)
# - Plot Pm2.5atm vs Time for all three rooms on the same graph
#   - For this first make it so that we have avrage value of PM2.5atm per minute
#   - then plot new PM2.5atm vs time one sub plot for each room
#   - save it at `data/Dhruv_Patel/Plots/Multi_Day_Time_Series`


# --- Markdown cell 8 ---
# ### Remove outliers


# --- Code cell 9 ---
def remove_outliers_iqr(df, column, threshold):
    """
    Filter out rows where a column exceeds a fixed threshold.

    Note: despite the name, this is a simple threshold filter (not true IQR).
    """
    # Keep only values at or below the supplied cutoff.
    df_clean = df[df[column] <= threshold].copy()
    return df_clean


# --- Code cell 10 ---
def multi_day_time_series(
    kitchen_df=None, hall_df=None, bedroom_df=None, outdoor_df=None
):
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if outdoor_df is None:
        raise ValueError("outdoor_df is None")

    # Extract DateTime and PM2.5 columns for each room
    bedroom_df_subset = bedroom_df[["pm2.5atm"]].copy()
    hall_df_subset = hall_df[["pm2.5atm"]].copy()
    kitchen_df_subset = kitchen_df[["pm2.5atm"]].copy()
    outdoor_df_subset = outdoor_df.copy()

    # Convert DateTime to datetime if not already
    bedroom_df_subset.index = pd.to_datetime(bedroom_df_subset.index)
    hall_df_subset.index = pd.to_datetime(hall_df_subset.index)
    kitchen_df_subset.index = pd.to_datetime(kitchen_df_subset.index)

    # Log scale requires positive values
    kitchen_series = kitchen_df_subset["pm2.5atm"].where(
        kitchen_df_subset["pm2.5atm"] > 0
    )
    hall_series = hall_df_subset["pm2.5atm"].where(hall_df_subset["pm2.5atm"] > 0)
    bedroom_series = bedroom_df_subset["pm2.5atm"].where(
        bedroom_df_subset["pm2.5atm"] > 0
    )
    outdoor_df_subset = outdoor_df_subset.copy()
    # Convert outdoor pm25 to numeric and filter positive values
    outdoor_df_subset["pm25"] = pd.to_numeric(
        outdoor_df_subset["pm25"], errors="coerce"
    )
    outdoor_df_subset["pm25"] = outdoor_df_subset["pm25"].where(
        outdoor_df_subset["pm25"] > 0
    )

    # Plot the data for each room in a single plot
    plt.figure(figsize=(15, 8))
    plt.plot(
        kitchen_df_subset.index,
        kitchen_series,
        color=kitchen_colour,
        label="Kitchen",
        alpha=0.8,
    )
    plt.plot(
        hall_df_subset.index,
        hall_series,
        color=hall_colour,
        label="Hall",
        alpha=0.8,
    )
    plt.plot(
        bedroom_df_subset.index,
        bedroom_series,
        color=bedroom_colour,
        label="Bedroom",
        alpha=0.8,
    )
    plt.plot(
        outdoor_df_subset.index,
        outdoor_df_subset["pm25"],
        color=outdoor_colour,
        label="Outdoor",
        alpha=1.0,
        linewidth=4,
    )
    plt.yscale("log")
    plt.xlabel("Date")
    plt.ylabel("PM2.5 (µg/m³, log scale)")
    plt.title("Multi-Day Time Series of PM2.5")
    plt.legend()
    plt.grid(alpha=0.3, which="both")
    # Format x-axis to show date only (use weekly intervals to avoid too many ticks)
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    plt.gca().xaxis.set_major_locator(mdates.DayLocator(interval=1))
    plt.xticks(rotation=45)
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Multi_Day_Time_Series"), exist_ok=True)
    plt.savefig(
        os.path.join(PLOT_DIR, "Multi_Day_Time_Series", "Multi_Day_Time_Series.png")
    )

    # Show the plot
    plt.show()


# --- Markdown cell 12 ---
# ## Daily Average Trend Plot (Very Important)
# - For each day compute:
#   - Daily mean PM2.5
#   - Daily max PM2.5
#
# - Plot daily mean & max Vs date in same graph


# --- Code cell 13 ---
def daily_average_trend_plot(kitchen_df=None, hall_df=None, bedroom_df=None):
    """
    Plot daily mean and daily max PM2.5 for each room.

    This summarizes long-term trends and highlights extreme days.
    """
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Keep only time and PM2.5 columns for resampling.
    bedroom_df_subset = bedroom_df[["pm2.5atm"]]
    hall_df_subset = hall_df[["pm2.5atm"]]
    kitchen_df_subset = kitchen_df[["pm2.5atm"]]

    # Resample to daily averages and daily maxima.
    bedroom_daily_avg = bedroom_df_subset.resample("D").mean()
    bedroom_daily_max = bedroom_df_subset.resample("D").max()

    hall_daily_avg = hall_df_subset.resample("D").mean()
    hall_daily_max = hall_df_subset.resample("D").max()

    kitchen_daily_avg = kitchen_df_subset.resample("D").mean()
    kitchen_daily_max = kitchen_df_subset.resample("D").max()

    # Plot the daily average and max in stacked subplots for easy comparison.
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
    ax1.plot(
        bedroom_daily_avg.index,
        bedroom_daily_avg["pm2.5atm"],
        label="Bedroom Daily Average",
        alpha=0.7,
        color="blue",
    )
    ax1.plot(
        bedroom_daily_max.index,
        bedroom_daily_max["pm2.5atm"],
        label="Bedroom Daily Max",
        alpha=0.7,
        color="red",
    )
    ax1.set_ylabel("Daily Average and Max of pm2.5atm")
    ax1.set_title("Bedroom")
    ax1.legend()
    ax1.grid(alpha=0.3)

    ax2.plot(
        hall_daily_avg.index,
        hall_daily_avg["pm2.5atm"],
        label="Hall Daily Average",
        alpha=0.7,
        color="orange",
    )
    ax2.plot(
        hall_daily_max.index,
        hall_daily_max["pm2.5atm"],
        label="Hall Daily Max",
        alpha=0.7,
        color="red",
    )
    ax2.set_ylabel("Daily Average and Max of pm2.5atm")
    ax2.set_title("Hall")
    ax2.legend()
    ax2.grid(alpha=0.3)

    ax3.plot(
        kitchen_daily_avg.index,
        kitchen_daily_avg["pm2.5atm"],
        label="Kitchen Daily Average",
        alpha=0.7,
        color="green",
    )
    ax3.plot(
        kitchen_daily_max.index,
        kitchen_daily_max["pm2.5atm"],
        label="Kitchen Daily Max",
        alpha=0.7,
        color="red",
    )
    ax3.set_ylabel("Daily Average and Max of pm2.5atm")
    ax3.set_title("Kitchen")
    ax3.legend()
    ax3.grid(alpha=0.3)
    plt.suptitle("Daily Average and Max of pm2.5atm for Each Room")
    plt.tight_layout()
    # Save the plot
    os.makedirs(f"{PLOT_DIR}/Daily_Average_Trend_Plot", exist_ok=True)
    plt.savefig(
        f"{PLOT_DIR}/Daily_Average_Trend_Plot/pm25_daily_avg_max.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()


# --- Markdown cell 15 ---
# ## Boxplot Across Days (Strong Academic Figure)
# - Create box graph per room
#   - Per room per day
# - This shows:
#   - Day-to-day variability
#   - Extreme pollution events
#   - Consistency of exposure


# --- Markdown cell 16 ---
# ### Function to plot data


# --- Code cell 17 ---
# Alternative version with enhanced statistical information
def create_daily_boxplot(kitchen_df=None, hall_df=None, bedroom_df=None):
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")

    # Create a combined DataFrame for boxplot
    kitchen_reset = kitchen_df.reset_index().copy()
    hall_reset = hall_df.reset_index().copy()
    bedroom_reset = bedroom_df.reset_index().copy()
    kitchen_reset["Room"] = "Kitchen"
    hall_reset["Room"] = "Hall"
    bedroom_reset["Room"] = "Bedroom"
    combined_df = pd.concat(
        [
            kitchen_reset[["pm2.5atm", "Room"]],
            hall_reset[["pm2.5atm", "Room"]],
            bedroom_reset[["pm2.5atm", "Room"]],
        ],
        ignore_index=True,
    )

    plt.figure(figsize=(12, 8))
    ax = sns.boxplot(
        x="Room",
        y="pm2.5atm",
        hue="Room",
        data=combined_df,
        palette=[kitchen_colour, hall_colour, bedroom_colour],
        legend=False,
    )

    # Add average lines for each room (full width)
    room_averages = combined_df.groupby("Room")["pm2.5atm"].mean()
    room_positions = {"Kitchen": 0, "Hall": 1, "Bedroom": 2}
    room_colors = {
        "Kitchen": kitchen_colour,
        "Hall": hall_colour,
        "Bedroom": bedroom_colour,
    }

    for room, avg_val in room_averages.items():
        pos = room_positions[room]
        color = room_colors[room]
        # ax.plot([pos - 0.4, pos + 0.4], [avg_val, avg_val], color=color, linestyle="--", linewidth=3, label=f"{room} Avg: {avg_val:.1f}")

    # Add lines showing average PM2.5 values for each room with labels
    for i, (room, avg_val) in enumerate(room_averages.items()):
        plt.axhline(
            y=avg_val,
            color=room_colors[room],
            linestyle="--",
            linewidth=3,
            label=f"{room} Avg: {avg_val:.1f}",
        )
    plt.yscale("log")
    plt.ylabel("PM2.5 (µg/m³, log scale)", fontsize=14, fontweight="bold")
    plt.xlabel("Room", fontsize=14, fontweight="bold")
    plt.title("Combined Daily Boxplot of PM2.5 by Room", fontsize=16, fontweight="bold")
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.legend(fontsize=11, loc="upper right")
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Daily_Boxplot"), exist_ok=True)
    plt.savefig(os.path.join(PLOT_DIR, "Daily_Boxplot", "Boxplot_Combined.png"))
    # Show the plot
    plt.show()

    # Extract Date from DateTime column for each room and calculate daily average pm2.5atm
    kitchen_daily = kitchen_df.reset_index().copy()
    hall_daily = hall_df.reset_index().copy()
    bedroom_daily = bedroom_df.reset_index().copy()

    kitchen_daily["Day"] = pd.to_datetime(kitchen_daily["DateTime"]).dt.day.astype(str)
    hall_daily["Day"] = pd.to_datetime(hall_daily["DateTime"]).dt.day.astype(str)
    bedroom_daily["Day"] = pd.to_datetime(bedroom_daily["DateTime"]).dt.day.astype(str)

    # Store the dataframes in a dictionary for easy access
    room_dataframes = {
        "Kitchen": kitchen_daily,
        "Hall": hall_daily,
        "Bedroom": bedroom_daily,
    }
    room_colors = {
        "Kitchen": kitchen_colour,
        "Hall": hall_colour,
        "Bedroom": bedroom_colour,
    }
    rooms = ["Kitchen", "Hall", "Bedroom"]

    # Create separate plots for each room
    for room in rooms:
        # Get data for current room from the corresponding dataframe
        room_data = room_dataframes[room]

        # Main boxplot
        daily_data = []
        day_labels = []
        daily_stats = []

        for day in sorted(room_data["Day"].unique(), key=int):
            day_values = room_data[room_data["Day"] == day]["pm2.5atm"].values
            if len(day_values) > 0:
                daily_data.append(day_values)
                day_labels.append(f"{day}")
                # Calculate daily statistics
                daily_stats.append(
                    {
                        "mean": np.mean(day_values),
                        "std": np.std(day_values),
                        "cv": np.std(day_values) / np.mean(day_values) * 100,
                        "max": np.max(day_values),
                    }
                )

        # Create separate figure for each room
        fig, ax = plt.subplots(figsize=(16, 10))
        bp = ax.boxplot(
            daily_data, tick_labels=day_labels, patch_artist=True, widths=0.6
        )

        # Color boxes based on daily mean (gradient from low to high)
        daily_means = [stats["mean"] for stats in daily_stats]
        if daily_means:
            norm = Normalize(vmin=min(daily_means), vmax=max(daily_means))
            cmap = plt.get_cmap("Reds")

            for patch, mean_val in zip(bp["boxes"], daily_means):
                patch.set_facecolor(cmap(norm(mean_val)))
                patch.set_alpha(0.8)
                patch.set_linewidth(2)

        # Increase whisker and median line widths
        for whisker in bp["whiskers"]:
            whisker.set_linewidth(2)
        for cap in bp["caps"]:
            cap.set_linewidth(2)
        for median in bp["medians"]:
            median.set_linewidth(3)
            median.set_color(room_colors[room])

        ax.set_title(
            f"{room} - Daily PM2.5 Distribution", fontweight="bold", fontsize=18
        )
        ax.set_xlabel("Day", fontsize=16, fontweight="bold")
        ax.set_ylabel("PM2.5 (µg/m³, log scale)", fontsize=16, fontweight="bold")
        ax.set_yscale("log")
        ax.grid(True, alpha=0.3, which="both")
        ax.tick_params(axis="x", labelsize=13)
        ax.tick_params(axis="y", labelsize=13)

        fig.tight_layout()
        # Save the plot
        os.makedirs(os.path.join(PLOT_DIR, "Diurnal_Pattern"), exist_ok=True)
        fig.savefig(
            os.path.join(PLOT_DIR, "Diurnal_Pattern", f"Daily_Boxplot_{room}.png"),
            dpi=300,
            bbox_inches="tight",
        )
        # Show the plot
        plt.show()


# --- Markdown cell 19 ---
# ## Diurnal Pattern (MOST IMPORTANT FOR SOURCE IDENTIFICATION)
# #### Step:
# - Extract hour from DateTime
# - Compute average PM2.5 for each hour (across all days)
#
# #### Plot:
# - Hour (0–23) vs Average PM2.5
# - Make separate curves for:
#   - Bedroom
#   - Hall
#   - Kitchen


# --- Code cell 20 ---
def diurnal_pattern(kitchen_df=None, hall_df=None, bedroom_df=None):
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Reset index to convert DateTime from index to column
    bedroom_df = bedroom_df.reset_index().copy()
    hall_df = hall_df.reset_index().copy()
    kitchen_df = kitchen_df.reset_index().copy()

    # Create a list of only the pm2.5atm and DateTime columns for each room
    bedroom_hourly_df = bedroom_df[["DateTime", "pm2.5atm"]].copy()
    hall_hourly_df = hall_df[["DateTime", "pm2.5atm"]].copy()
    kitchen_hourly_df = kitchen_df[["DateTime", "pm2.5atm"]].copy()

    # Convert DateTime to datetime if not already
    bedroom_hourly_df["DateTime"] = pd.to_datetime(bedroom_hourly_df["DateTime"])
    hall_hourly_df["DateTime"] = pd.to_datetime(hall_hourly_df["DateTime"])
    kitchen_hourly_df["DateTime"] = pd.to_datetime(kitchen_hourly_df["DateTime"])

    # Create a new column for the hour of the day in each dataframe
    bedroom_hourly_df["Hour"] = bedroom_hourly_df["DateTime"].dt.hour
    hall_hourly_df["Hour"] = hall_hourly_df["DateTime"].dt.hour
    kitchen_hourly_df["Hour"] = kitchen_hourly_df["DateTime"].dt.hour

    # Calculate the average `pm2.5atm` for each hour of the day for each room
    bedroom_hourly_avg = bedroom_hourly_df.groupby("Hour")["pm2.5atm"].mean()
    hall_hourly_avg = hall_hourly_df.groupby("Hour")["pm2.5atm"].mean()
    kitchen_hourly_avg = kitchen_hourly_df.groupby("Hour")["pm2.5atm"].mean()

    # Print average of pm2.5atm between 5am to 6am and 4pm to 5pm for each room
    print("Average PM2.5 between 5am to 6am:")
    print(f"  Kitchen: {kitchen_hourly_avg.get(5, np.nan):.2f} µg/m³")
    print(f"  Hall: {hall_hourly_avg.get(5, np.nan):.2f} µg/m³")
    print(f"  Bedroom: {bedroom_hourly_avg.get(5, np.nan):.2f} µg/m³")
    print("Average PM2.5 between 4pm to 5pm:")
    print(f"  Kitchen: {kitchen_hourly_avg.get(16, np.nan):.2f} µg/m³")
    print(f"  Hall: {hall_hourly_avg.get(16, np.nan):.2f} µg/m³")
    print(f"  Bedroom: {bedroom_hourly_avg.get(16, np.nan):.2f} µg/m³")
    print("Average PM2.5 between 5am to 6am and 4pm to 5pm:")
    kitchen_base_avg = (
        kitchen_hourly_avg.get(5, np.nan) + kitchen_hourly_avg.get(16, np.nan)
    ) / 2
    hall_base_avg = (
        hall_hourly_avg.get(5, np.nan) + hall_hourly_avg.get(16, np.nan)
    ) / 2
    bedroom_base_avg = (
        bedroom_hourly_avg.get(5, np.nan) + bedroom_hourly_avg.get(16, np.nan)
    ) / 2
    print(f"  Kitchen: {kitchen_base_avg:.2f} µg/m³")
    print(f"  Hall: {hall_base_avg:.2f} µg/m³")
    print(f"  Bedroom: {bedroom_base_avg:.2f} µg/m³")

    # Plot the diurnal pattern for each room
    plt.figure(figsize=(15, 6))
    plt.plot(
        kitchen_hourly_avg.index,
        kitchen_hourly_avg.values,
        color=kitchen_colour,
        label="Kitchen",
        alpha=0.8,
    )
    plt.plot(
        hall_hourly_avg.index,
        hall_hourly_avg.values,
        color=hall_colour,
        label="Hall",
        alpha=0.8,
    )
    plt.plot(
        bedroom_hourly_avg.index,
        bedroom_hourly_avg.values,
        color=bedroom_colour,
        label="Bedroom",
        alpha=0.8,
    )
    plt.xlabel("Hour of the Day")
    plt.xticks(range(0, 24, 1))
    plt.ylabel("Average PM2.5 (µg/m³)")
    plt.title("Diurnal Pattern of PM2.5")
    plt.legend()
    plt.grid(alpha=0.3)
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Diurnal_Pattern"), exist_ok=True)
    plt.savefig(os.path.join(PLOT_DIR, "Diurnal_Pattern", "Diurnal_Pattern.png"))
    # Show the plot
    plt.show()
    return kitchen_base_avg, hall_base_avg, bedroom_base_avg


# --- Markdown cell 22 ---
# ## Weekend vs Weekday Comparison (Advanced)
# #### Steps
# - Add Column `WeekDay` (i.e 1, 2, 3, 4, 5, 6, 7) to all rooms data
# - Compute average PM2.5 for:
#   - Weekdays (1–5)
#   - Weekends (6–7)
# #### Plot
# - Bar graph comparing average PM2.5 for Weekdays vs Weekends for each room


# --- Code cell 23 ---
def weekdays_vs_weekends_comparison(kitchen_df=None, hall_df=None, bedroom_df=None):
    """
    Compare average PM2.5 on weekdays versus weekends for each room.

    Aggregates by day-of-week and then collapses into Weekday vs Weekend.
    """
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Reset index to convert DateTime from index to column
    bedroom_df = bedroom_df.reset_index().copy()
    hall_df = hall_df.reset_index().copy()
    kitchen_df = kitchen_df.reset_index().copy()

    # Keep only time and PM2.5 columns for weekday grouping.
    bedroom_weekly_df = bedroom_df[["DateTime", "pm2.5atm"]]
    hall_weekly_df = hall_df[["DateTime", "pm2.5atm"]]
    kitchen_weekly_df = kitchen_df[["DateTime", "pm2.5atm"]]

    # Remove outliers before weekday aggregation.
    bedroom_weekly_df = remove_outliers_iqr(bedroom_weekly_df, "pm2.5atm", 200)
    hall_weekly_df = remove_outliers_iqr(hall_weekly_df, "pm2.5atm", 1500)
    kitchen_weekly_df = remove_outliers_iqr(kitchen_weekly_df, "pm2.5atm", 1500)

    # Create a new column for the weekday of the year in each dataframe
    bedroom_weekly_df["Weekday"] = bedroom_weekly_df["DateTime"].dt.dayofweek
    hall_weekly_df["Weekday"] = hall_weekly_df["DateTime"].dt.dayofweek
    kitchen_weekly_df["Weekday"] = kitchen_weekly_df["DateTime"].dt.dayofweek

    # Calculate average PM2.5 for each weekday.
    bedroom_weekly_avg = bedroom_weekly_df.groupby("Weekday")["pm2.5atm"].mean()
    hall_weekly_avg = hall_weekly_df.groupby("Weekday")["pm2.5atm"].mean()
    kitchen_weekly_avg = kitchen_weekly_df.groupby("Weekday")["pm2.5atm"].mean()

    # Collapse into Weekday (Mon–Fri) vs Weekend (Sat–Sun).
    bedroom_weekly_avg["Weekend"] = bedroom_weekly_df[
        bedroom_weekly_df["Weekday"] >= 5
    ]["pm2.5atm"].mean()
    bedroom_weekly_avg["Weekday"] = bedroom_weekly_df[bedroom_weekly_df["Weekday"] < 5][
        "pm2.5atm"
    ].mean()
    hall_weekly_avg["Weekend"] = hall_weekly_df[hall_weekly_df["Weekday"] >= 5][
        "pm2.5atm"
    ].mean()
    hall_weekly_avg["Weekday"] = hall_weekly_df[hall_weekly_df["Weekday"] < 5][
        "pm2.5atm"
    ].mean()
    kitchen_weekly_avg["Weekend"] = kitchen_weekly_df[
        kitchen_weekly_df["Weekday"] >= 5
    ]["pm2.5atm"].mean()
    kitchen_weekly_avg["Weekday"] = kitchen_weekly_df[kitchen_weekly_df["Weekday"] < 5][
        "pm2.5atm"
    ].mean()

    # Plot the average `pm2.5atm` for Weekday vs Weekend for each room in a subplot with a shared x-axis
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
    ax1.bar(
        ["Weekday", "Weekend"],
        [bedroom_weekly_avg["Weekday"], bedroom_weekly_avg["Weekend"]],
        color=["green", "lightgreen"],
    )
    ax2.bar(
        ["Weekday", "Weekend"],
        [hall_weekly_avg["Weekday"], hall_weekly_avg["Weekend"]],
        color=["green", "lightgreen"],
    )
    ax3.bar(
        ["Weekday", "Weekend"],
        [kitchen_weekly_avg["Weekday"], kitchen_weekly_avg["Weekend"]],
        color=["green", "lightgreen"],
    )
    ax1.set_ylabel("Average pm2.5atm")
    ax1.set_title("Average pm2.5atm - Bedroom")
    ax1.grid(alpha=0.5)
    ax1.set_yticks(
        np.arange(
            0,
            max(bedroom_weekly_avg["Weekday"], bedroom_weekly_avg["Weekend"]) + 20,
            10,
        )
    )
    ax2.set_ylabel("Average pm2.5atm")
    ax2.set_title("Average pm2.5atm - Hall")
    ax2.grid(alpha=0.5)
    ax2.set_yticks(
        np.arange(
            0, max(hall_weekly_avg["Weekday"], hall_weekly_avg["Weekend"]) + 20, 10
        )
    )
    ax3.set_ylabel("Average pm2.5atm")
    ax3.set_title("Average pm2.5atm - Kitchen")
    ax3.grid(alpha=0.5)
    ax3.set_yticks(
        np.arange(
            0,
            max(kitchen_weekly_avg["Weekday"], kitchen_weekly_avg["Weekend"]) + 20,
            10,
        )
    )
    ax3.set_xlabel("Day Type")
    plt.suptitle("Average pm2.5atm for Weekday vs Weekend for Each Room")
    plt.tight_layout()

    # Save the plot
    os.makedirs(f"{PLOT_DIR}/Weekday_vs_Weekend_Comparison", exist_ok=True)
    plt.savefig(
        f"{PLOT_DIR}/Weekday_vs_Weekend_Comparison/weekday_weekend_comparison.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()


# --- Markdown cell 25 ---
# ## Percentage Time Above Guidelines (Exposure Burden)
# #### Calculate for each room:
# - % time `pm2.5atm` > 15 µg/m³ (WHO)
# - % time `pm2.5atm` > 60 µg/m³ (India 24-hr NAAQS)
# - % change change in `pm2.5atm` from the previous measurement for each room
# #### Plot
# - print % time `pm2.5atm` > 15 µg/m³ (WHO)
# - print% time `pm2.5atm` > 60 µg/m³ (India 24*hr NAAQS)
# - line graph showing '% change in `pm2.5atm` from the previous measurement for each room'


# --- Code cell 26 ---
def percentage_time_above_guidelines(kitchen_df=None, hall_df=None, bedroom_df=None):
    """
    Report and plot exposure burden relative to guideline thresholds.

    Calculates percent time above WHO 15 µg/m³ and India NAAQS 60 µg/m³,
    then visualizes percent change over time.
    """
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Reset index to convert DateTime from index to column
    bedroom_df = bedroom_df.reset_index().copy()
    hall_df = hall_df.reset_index().copy()
    kitchen_df = kitchen_df.reset_index().copy()

    # Keep only time and PM2.5 columns for threshold checks.
    bedroom_percent_df = bedroom_df[["DateTime", "pm2.5atm"]]
    hall_percent_df = hall_df[["DateTime", "pm2.5atm"]]
    kitchen_percent_df = kitchen_df[["DateTime", "pm2.5atm"]]

    # Remove outliers before calculating exceedance percentages.
    bedroom_percent_df = remove_outliers_iqr(bedroom_percent_df, "pm2.5atm", 200)
    hall_percent_df = remove_outliers_iqr(hall_percent_df, "pm2.5atm", 1500)
    kitchen_percent_df = remove_outliers_iqr(kitchen_percent_df, "pm2.5atm", 1500)

    # Percentage of time above WHO guideline (15 µg/m³).
    bedroom_percent_over_25 = (
        (bedroom_percent_df["pm2.5atm"] > 15).sum() / len(bedroom_percent_df) * 100
    )
    hall_percent_over_25 = (
        (hall_percent_df["pm2.5atm"] > 15).sum() / len(hall_percent_df) * 100
    )
    kitchen_percent_over_25 = (
        (kitchen_percent_df["pm2.5atm"] > 15).sum() / len(kitchen_percent_df) * 100
    )
    print(
        f"Percentage of time pm2.5atm exceeds 15 μg/m³ - Bedroom: {bedroom_percent_over_25:.2f}%"
    )
    print(
        f"Percentage of time pm2.5atm exceeds 15 μg/m³ - Hall: {hall_percent_over_25:.2f}%"
    )
    print(
        f"Percentage of time pm2.5atm exceeds 15 μg/m³ - Kitchen: {kitchen_percent_over_25:.2f}%"
    )

    # Percentage of time above India NAAQS 24-hr guideline (60 µg/m³).
    bedroom_percent_over_60 = (
        (bedroom_percent_df["pm2.5atm"] > 60).sum() / len(bedroom_percent_df) * 100
    )
    hall_percent_over_60 = (
        (hall_percent_df["pm2.5atm"] > 60).sum() / len(hall_percent_df) * 100
    )
    kitchen_percent_over_60 = (
        (kitchen_percent_df["pm2.5atm"] > 60).sum() / len(kitchen_percent_df) * 100
    )
    print(
        f"Percentage of time pm2.5atm exceeds 60 μg/m³ - Bedroom: {bedroom_percent_over_60:.2f}%"
    )
    print(
        f"Percentage of time pm2.5atm exceeds 60 μg/m³ - Hall: {hall_percent_over_60:.2f}%"
    )
    print(
        f"Percentage of time pm2.5atm exceeds 60 μg/m³ - Kitchen: {kitchen_percent_over_60:.2f}%"
    )

    # Percent change highlights sudden spikes or drops between consecutive readings.
    bedroom_percent_df["Percent Change"] = (
        bedroom_percent_df["pm2.5atm"].pct_change() * 100
    )
    hall_percent_df["Percent Change"] = hall_percent_df["pm2.5atm"].pct_change() * 100
    kitchen_percent_df["Percent Change"] = (
        kitchen_percent_df["pm2.5atm"].pct_change() * 100
    )

    # Plot the percent change in `pm2.5atm` for each room in a subplot with a shared x-axis
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(18, 10), sharex=True)
    ax1.plot(
        bedroom_percent_df["DateTime"],
        bedroom_percent_df["Percent Change"],
        label="Bedroom",
        color="blue",
    )
    ax1.set_ylabel("Percent Change in pm2.5atm")
    ax1.set_title("Percent Change in pm2.5atm - Bedroom")
    ax1.legend()
    ax1.grid(alpha=0.3)

    ax2.plot(
        hall_percent_df["DateTime"],
        hall_percent_df["Percent Change"],
        label="Hall",
        color="orange",
    )
    ax2.set_ylabel("Percent Change in pm2.5atm")
    ax2.set_title("Percent Change in pm2.5atm - Hall")
    ax2.legend()
    ax2.grid(alpha=0.3)

    ax3.plot(
        kitchen_percent_df["DateTime"],
        kitchen_percent_df["Percent Change"],
        label="Kitchen",
        color="green",
    )
    ax3.set_xlabel("DateTime")
    ax3.set_ylabel("Percent Change in pm2.5atm")
    ax3.set_title("Percent Change in pm2.5atm - Kitchen")
    ax3.legend()
    ax3.grid(alpha=0.3)
    plt.suptitle("Percent Change in pm2.5atm for Each Room")

    # Save the plot
    os.makedirs(f"{PLOT_DIR}/Percent_Change_Trend_Plot", exist_ok=True)
    plt.savefig(
        f"{PLOT_DIR}/Percent_Change_Trend_Plot/percent_change_trend.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()


# --- Markdown cell 28 ---
# ## Peak Event Analysis
# #### Define peak event thresholds
# - Kitchen & Hall > 300 µg/m³
# - Bedroom > 150 µg/m³
# #### Calculations:
# - For each day calculate:
#   - Number of peaks
#   - Duration of peaks
#   - Maximum peak concentration
#   - Note for each rooms


# --- Code cell 29 ---
def peak_event_analysis(
    kitchen_df=None,
    hall_df=None,
    bedroom_df=None,
    kitchen_base_avg=None,
    hall_base_avg=None,
    bedroom_base_avg=None,
):
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")
    if kitchen_base_avg is None:
        raise ValueError("kitchen_base_avg is None")
    if hall_base_avg is None:
        raise ValueError("hall_base_avg is None")
    if bedroom_base_avg is None:
        raise ValueError("bedroom_base_avg is None")

    # Reset index to convert DateTime from index to column
    kitchen_df = kitchen_df.reset_index().copy()
    hall_df = hall_df.reset_index().copy()
    bedroom_df = bedroom_df.reset_index().copy()

    # Create a list of only the 'DateTime' and `pm2.5atm` columns for each room
    kitchen_peak_df = kitchen_df[["DateTime", "pm2.5atm"]].copy()
    hall_peak_df = hall_df[["DateTime", "pm2.5atm"]].copy()
    bedroom_peak_df = bedroom_df[["DateTime", "pm2.5atm"]].copy()

    # Convert DateTime to datetime type and set as index
    kitchen_peak_df["DateTime"] = pd.to_datetime(kitchen_peak_df["DateTime"])
    hall_peak_df["DateTime"] = pd.to_datetime(hall_peak_df["DateTime"])
    bedroom_peak_df["DateTime"] = pd.to_datetime(bedroom_peak_df["DateTime"])

    kitchen_peak_df.set_index("DateTime", inplace=True)
    hall_peak_df.set_index("DateTime", inplace=True)
    bedroom_peak_df.set_index("DateTime", inplace=True)

    # Create a new column to identify peak events for each room based provided base average (average of 5am-6am and 4pm-5pm) for each room. A peak event is defined as a time when `pm2.5atm` exceeds the base average for that room.
    kitchen_peak_df["Peak Event"] = kitchen_peak_df["pm2.5atm"] > kitchen_base_avg
    hall_peak_df["Peak Event"] = hall_peak_df["pm2.5atm"] > hall_base_avg
    bedroom_peak_df["Peak Event"] = bedroom_peak_df["pm2.5atm"] > bedroom_base_avg

    # Ensure that data frames are sorted by DateTime
    kitchen_peak_df.sort_index(inplace=True)
    hall_peak_df.sort_index(inplace=True)
    bedroom_peak_df.sort_index(inplace=True)

    # Create a new column for the date (without time) in each dataframe
    kitchen_peak_df["Date"] = kitchen_peak_df.index.date
    hall_peak_df["Date"] = hall_peak_df.index.date
    bedroom_peak_df["Date"] = bedroom_peak_df.index.date

    # Identify Peack Start
    bedroom_peak_df["Peak Start"] = (bedroom_peak_df["Peak Event"] == True) & (
        bedroom_peak_df["Peak Event"].shift(1) == False
    )
    hall_peak_df["Peak Start"] = (hall_peak_df["Peak Event"] == True) & (
        hall_peak_df["Peak Event"].shift(1) == False
    )
    kitchen_peak_df["Peak Start"] = (kitchen_peak_df["Peak Event"] == True) & (
        kitchen_peak_df["Peak Event"].shift(1) == False
    )

    # Assign a unique ID to each peak event
    bedroom_peak_df["Peak ID"] = bedroom_peak_df["Peak Start"].cumsum()
    hall_peak_df["Peak ID"] = hall_peak_df["Peak Start"].cumsum()
    kitchen_peak_df["Peak ID"] = kitchen_peak_df["Peak Start"].cumsum()

    # All days in the dataset for each room to ensure we keep days with no peak events in the analysis
    all_days = pd.date_range(
        start=bedroom_peak_df["Date"].min(), end=bedroom_peak_df["Date"].max()
    ).date

    # Remove non peak events
    bedroom_peak_df = bedroom_peak_df[bedroom_peak_df["Peak Event"] == True]
    hall_peak_df = hall_peak_df[hall_peak_df["Peak Event"] == True]
    kitchen_peak_df = kitchen_peak_df[kitchen_peak_df["Peak Event"] == True]

    # Count number of peak events per day for each room
    bedroom_peak_events_per_day = (
        bedroom_peak_df.groupby("Date")["Peak ID"]
        .nunique()
        .reset_index(name="Peak Events")
    )
    hall_peak_events_per_day = (
        hall_peak_df.groupby("Date")["Peak ID"]
        .nunique()
        .reset_index(name="Peak Events")
    )
    kitchen_peak_events_per_day = (
        kitchen_peak_df.groupby("Date")["Peak ID"]
        .nunique()
        .reset_index(name="Peak Events")
    )
    # Keep days with no peak events as well
    bedroom_peak_events_per_day = (
        bedroom_peak_events_per_day.set_index("Date")
        .reindex(all_days, fill_value=0)
        .reset_index()
        .rename(columns={"index": "Date"})
    )
    hall_peak_events_per_day = (
        hall_peak_events_per_day.set_index("Date")
        .reindex(all_days, fill_value=0)
        .reset_index()
        .rename(columns={"index": "Date"})
    )
    kitchen_peak_events_per_day = (
        kitchen_peak_events_per_day.set_index("Date")
        .reindex(all_days, fill_value=0)
        .reset_index()
        .rename(columns={"index": "Date"})
    )

    # Calculate Duration of Each Peak Event with Day
    bedroom_peak_duration = (
        bedroom_peak_df.reset_index()
        .groupby(["Date", "Peak ID"])
        .agg(min_time=("DateTime", "min"), max_time=("DateTime", "max"))
        .reset_index()
    )
    hall_peak_duration = (
        hall_peak_df.reset_index()
        .groupby(["Date", "Peak ID"])
        .agg(min_time=("DateTime", "min"), max_time=("DateTime", "max"))
        .reset_index()
    )
    kitchen_peak_duration = (
        kitchen_peak_df.reset_index()
        .groupby(["Date", "Peak ID"])
        .agg(min_time=("DateTime", "min"), max_time=("DateTime", "max"))
        .reset_index()
    )
    bedroom_peak_duration["Duration (minutes)"] = (
        bedroom_peak_duration["max_time"] - bedroom_peak_duration["min_time"]
    ).dt.total_seconds() / 60
    hall_peak_duration["Duration (minutes)"] = (
        hall_peak_duration["max_time"] - hall_peak_duration["min_time"]
    ).dt.total_seconds() / 60
    kitchen_peak_duration["Duration (minutes)"] = (
        kitchen_peak_duration["max_time"] - kitchen_peak_duration["min_time"]
    ).dt.total_seconds() / 60
    # Replace 0.0 with 1/60 to avoid zero duration
    bedroom_peak_duration["Duration (minutes)"] = bedroom_peak_duration[
        "Duration (minutes)"
    ].replace(0.0, 1 / 60)
    hall_peak_duration["Duration (minutes)"] = hall_peak_duration[
        "Duration (minutes)"
    ].replace(0.0, 1 / 60)
    kitchen_peak_duration["Duration (minutes)"] = kitchen_peak_duration[
        "Duration (minutes)"
    ].replace(0.0, 1 / 60)

    # Maximum Peak Concentration Per Day
    bedroom_peak_max = (
        bedroom_peak_df.groupby("Date")["pm2.5atm"]
        .max()
        .reset_index(name="Max pm2.5atm")
    )
    hall_peak_max = (
        hall_peak_df.groupby("Date")["pm2.5atm"].max().reset_index(name="Max pm2.5atm")
    )
    kitchen_peak_max = (
        kitchen_peak_df.groupby("Date")["pm2.5atm"]
        .max()
        .reset_index(name="Max pm2.5atm")
    )

    # Save the peak event analysis results to CSV files
    os.makedirs(f"{PLOT_DIR}/Peak_Event_Analysis", exist_ok=True)

    # Number of peak events per day for each room
    bedroom_peak_events_per_day.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/bedroom_peak_events_per_day.csv", index=False
    )
    hall_peak_events_per_day.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/hall_peak_events_per_day.csv", index=False
    )
    kitchen_peak_events_per_day.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/kitchen_peak_events_per_day.csv", index=False
    )

    # Duration of each peak event for each room
    bedroom_peak_duration.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/bedroom_peak_duration.csv", index=False
    )
    hall_peak_duration.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/hall_peak_duration.csv", index=False
    )
    kitchen_peak_duration.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/kitchen_peak_duration.csv", index=False
    )

    # Maximum peak concentration per day for each room
    bedroom_peak_max.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/bedroom_peak_max.csv", index=False
    )
    hall_peak_max.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/hall_peak_max.csv", index=False
    )
    kitchen_peak_max.to_csv(
        f"{PLOT_DIR}/Peak_Event_Analysis/kitchen_peak_max.csv", index=False
    )

    # For each room print total number of peak events, average duration of peak events, max duration of peak events, and average maximum concentration, max maximum concentration during peak events
    # Bedroom
    print(
        f"Total number of peak events - Bedroom: {bedroom_peak_df['Peak ID'].nunique()}"
    )
    print(
        f"Average duration of peak events (minutes) - Bedroom: {bedroom_peak_duration['Duration (minutes)'].mean():.2f}"
    )
    print(
        f"Max duration of peak events (minutes) - Bedroom: {bedroom_peak_duration['Duration (minutes)'].max():.2f}"
    )
    print(
        f"Average maximum concentration during peak events (µg/m³) - Bedroom: {bedroom_peak_max['Max pm2.5atm'].mean():.2f}"
    )
    print(
        f"Max maximum concentration during peak events (µg/m³) - Bedroom: {bedroom_peak_max['Max pm2.5atm'].max():.2f}"
    )
    # Hall
    print(f"\nTotal number of peak events - Hall: {hall_peak_df['Peak ID'].nunique()}")
    print(
        f"Average duration of peak events (minutes) - Hall: {hall_peak_duration['Duration (minutes)'].mean():.2f}"
    )
    print(
        f"Max duration of peak events (minutes) - Hall: {hall_peak_duration['Duration (minutes)'].max():.2f}"
    )
    print(
        f"Average maximum concentration during peak events (µg/m³) - Hall: {hall_peak_max['Max pm2.5atm'].mean():.2f}"
    )
    print(
        f"Max maximum concentration during peak events (µg/m³) - Hall: {hall_peak_max['Max pm2.5atm'].max():.2f}"
    )
    # Kitchen
    print(
        f"\nTotal number of peak events - Kitchen: {kitchen_peak_df['Peak ID'].nunique()}"
    )
    print(
        f"Average duration of peak events (minutes) - Kitchen: {kitchen_peak_duration['Duration (minutes)'].mean():.2f}"
    )
    print(
        f"Max duration of peak events (minutes) - Kitchen: {kitchen_peak_duration['Duration (minutes)'].max():.2f}"
    )
    print(
        f"Average maximum concentration during peak events (µg/m³) - Kitchen: {kitchen_peak_max['Max pm2.5atm'].mean():.2f}"
    )
    print(
        f"Max maximum concentration during peak events (µg/m³) - Kitchen: {kitchen_peak_max['Max pm2.5atm'].max():.2f}"
    )

    # Plot the number of peak events per day for each room
    plt.figure(figsize=(10, 5))
    plt.plot(
        kitchen_peak_events_per_day["Date"],
        kitchen_peak_events_per_day["Peak Events"],
        color=kitchen_colour,
        label="Kitchen",
        alpha=0.8,
    )
    plt.plot(
        hall_peak_events_per_day["Date"],
        hall_peak_events_per_day["Peak Events"],
        color=hall_colour,
        label="Hall",
        alpha=0.8,
    )
    plt.plot(
        bedroom_peak_events_per_day["Date"],
        bedroom_peak_events_per_day["Peak Events"],
        color=bedroom_colour,
        label="Bedroom",
        alpha=0.8,
    )
    plt.xlabel("Date")
    plt.ylabel("Number of Peak Events")
    plt.title("Number of Peak Events per Day")
    plt.legend()
    plt.grid(alpha=0.3)
    # Format x-axis to show date
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    plt.xticks(
        ticks=all_days,
        labels=[day.strftime("%Y-%m-%d") for day in all_days],
        rotation=75,
    )
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Peak_Event_Analysis"), exist_ok=True)
    plt.savefig(
        os.path.join(PLOT_DIR, "Peak_Event_Analysis", "Peak_Events_Per_Day.png")
    )
    # Show the plot
    plt.show()

    # Plot the Average & Max duration of peak events for each room
    plt.figure(figsize=(10, 5))
    plt.plot(
        kitchen_peak_duration.groupby("Date")["Duration (minutes)"].mean(),
        color=kitchen_colour,
        label="Kitchen - Average Duration",
        alpha=0.8,
    )
    plt.plot(
        hall_peak_duration.groupby("Date")["Duration (minutes)"].mean(),
        color=hall_colour,
        label="Hall - Average Duration",
        alpha=0.8,
    )
    plt.plot(
        bedroom_peak_duration.groupby("Date")["Duration (minutes)"].mean(),
        color=bedroom_colour,
        label="Bedroom - Average Duration",
        alpha=0.8,
    )
    plt.plot(
        kitchen_peak_duration.groupby("Date")["Duration (minutes)"].max(),
        color=kitchen_colour,
        linestyle="--",
        alpha=0.8,
        label="Kitchen - Max Duration",
    )
    plt.plot(
        hall_peak_duration.groupby("Date")["Duration (minutes)"].max(),
        color=hall_colour,
        linestyle="--",
        alpha=0.8,
        label="Hall - Max Duration",
    )
    plt.plot(
        bedroom_peak_duration.groupby("Date")["Duration (minutes)"].max(),
        color=bedroom_colour,
        linestyle="--",
        alpha=0.8,
        label="Bedroom - Max Duration",
    )
    plt.xlabel("Date")
    plt.ylabel("Duration of Peak Events (minutes)")
    plt.title("Duration of Peak Events per Day")
    plt.legend()
    plt.grid(alpha=0.3)
    # Format x-axis to show date
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    plt.xticks(
        ticks=all_days,
        labels=[day.strftime("%Y-%m-%d") for day in all_days],
        rotation=75,
    )
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Peak_Event_Analysis"), exist_ok=True)
    plt.savefig(
        os.path.join(PLOT_DIR, "Peak_Event_Analysis", "Peak_Event_Duration_Per_Day.png")
    )
    # Show the plot
    plt.show()

    # Plot the Average & Maximum concentration during peak events for each room
    plt.figure(figsize=(10, 5))
    plt.plot(
        kitchen_peak_max.groupby("Date")["Max pm2.5atm"].mean(),
        color=kitchen_colour,
        label="Kitchen - Average maximum Concentration",
        alpha=0.8,
    )
    plt.plot(
        hall_peak_max.groupby("Date")["Max pm2.5atm"].mean(),
        color=hall_colour,
        label="Hall - Average maximum Concentration",
        alpha=0.8,
    )
    plt.plot(
        bedroom_peak_max.groupby("Date")["Max pm2.5atm"].mean(),
        color=bedroom_colour,
        label="Bedroom - Average maximum Concentration",
        alpha=0.8,
    )
    plt.xlabel("Date")
    plt.ylabel("PM2.5 Concentration During Peak Events (µg/m³)")
    plt.title("Concentration During Peak Events per Day")
    plt.legend()
    plt.grid(alpha=0.3)
    # Format x-axis to show date
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    plt.xticks(
        ticks=all_days,
        labels=[day.strftime("%Y-%m-%d") for day in all_days],
        rotation=75,
    )
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Peak_Event_Analysis"), exist_ok=True)
    plt.savefig(
        os.path.join(
            PLOT_DIR, "Peak_Event_Analysis", "Peak_Event_Concentration_Per_Day.png"
        )
    )
    # Show the plot
    plt.show()


# --- Markdown cell 31 ---
# ## Cooking-Related PM2.5 Contribution Analysis
# #### Define cooking windows (This are general and most prefered not always correct)
# - Morning + Afternoon : 6:00 to 12:00 (From morning tea to Lunch)
# - Evening : 17:00 to 22:00 (From evening tea to Dinner)
# #### Steps
# - Crate a dataframe with `DateTime` and `pm2.5atm` as coloums
# - Add coloums `hour`, `minute`, `time`
# - Add coloums `Period` which classifies each entry as `Morning Cooking + Lunch`, `Evening Cooking + Dinner`, and `Non-Cooking`
# - Compare `Mean`, `Max`, `Standard Deviation` of `pm2.5atm` by Period
# - Compare Peak Frequency by Period
# - Exposure Contribution Analysis
#   - Calculate Total `pm2.5atm` during cooking vs total daily `pm2.5atm`
#   - Then compute percentage
# - Plot Diurnal Curve With Shaded Cooking Windows
#   - Plot hourly average `pm2.5atm`
#   - Then highlight Cooking durations
# - Room-Wise Cooking Impact
#   - Repeat analysis separately for:
#     - Kitchen
#     - Hall
#     - Bedroom


# --- Markdown cell 32 ---
# ### Helper Function


# --- Code cell 33 ---
from datetime import time


def classify_period(t):
    """
    Morning + Afternoon : 6:00 to 12:00 (From morning tea to Lunch)
    Evening : 17:00 to 22:00 (From evening tea to Dinner)
    """
    if time(6, 0) <= t <= time(12, 0):
        return "Morning Cooking + Lunch"
    elif time(17, 0) <= t <= time(22, 0):
        return "Evening Cooking + Dinner"
    else:
        return "Non-Cooking"


def get_total_hours(period):
    """Return total hours for each labeled period."""
    if period == "Morning Cooking + Lunch":
        return 6  # 6-12
    elif period == "Evening Cooking + Dinner":
        return 5  # 17-22
    else:
        return 13  # remaining hours


def calculate_cooking_related_PM2_5_contribution_analysis(
    room_name=None, room_df=None, room_colour=None, room_base_avg=None
):
    if room_df is None:
        raise ValueError("room_df is None")
    if room_name is None:
        raise ValueError("room_name is None")
    if room_colour is None:
        raise ValueError("room_colour is None")
    if room_base_avg is None:
        raise ValueError("room_base_avg is None")

    # Reset index to convert DateTime from index to column
    room_df = room_df.reset_index().copy()

    room_df = room_df[["DateTime", "pm2.5atm"]].copy()
    room_df["DateTime"] = pd.to_datetime(room_df["DateTime"])

    room_df["Time"] = room_df["DateTime"].dt.time
    room_df["Day"] = room_df["DateTime"].dt.strftime("%Y-%m-%d")
    room_df["Hour"] = room_df["DateTime"].dt.hour
    room_df["Minute"] = room_df["DateTime"].dt.minute

    room_df["Period"] = room_df["Time"].apply(classify_period)

    # Identify peak events based on room_base_avg (average of 5am-6am and 4pm-5pm) for the room. A peak event is defined as a time when `pm2.5atm` exceeds the base average for that room.
    room_df["Peak Event"] = room_df["pm2.5atm"] > room_base_avg

    # Compare `Mean`, `Max`, `Standard Deviation` of `pm2.5atm` by Period
    period_stats = (
        room_df.groupby("Period")["pm2.5atm"].agg(["mean", "max", "std"]).reset_index()
    )
    print(f"\nPM2.5 Contribution Analysis for {room_name}:")
    print(period_stats)

    # Compare peak frequency by Period
    peak_frequency = (
        room_df.groupby("Period")["Peak Event"].sum().reset_index(name="Peak Frequency")
    )
    print(f"\nPeak Frequency by Period for {room_name}:")
    print(peak_frequency)

    # Exposure contribution by Period (sum of pm2.5atm during peak events in each period)
    exposure_contribution_by_period = (
        room_df.groupby("Period")["pm2.5atm"]
        .sum()
        .reset_index(name="Exposure Contribution")
    )
    exposure_total = exposure_contribution_by_period["Exposure Contribution"].sum()
    exposure_contribution_by_period["Exposure Contribution (%)"] = (
        exposure_contribution_by_period["Exposure Contribution"] / exposure_total
    ) * 100

    exposure_contribution_by_period["Exposure Contribution (%) per hour"] = (
        exposure_contribution_by_period["Exposure Contribution (%)"]
        / get_total_hours(exposure_contribution_by_period["Period"].values[0])
    )
    print(f"\nExposure Contribution by Period for {room_name}:")
    print(exposure_contribution_by_period)

    # Plot Diurnal Curve With Shaded Cooking Windows
    # Plot hourly average `pm2.5atm`
    # Then highlight Cooking durations
    hourly_avg = room_df.groupby("Hour")["pm2.5atm"].mean().reset_index()
    plt.figure(figsize=(12, 6))
    plt.plot(
        hourly_avg["Hour"],
        hourly_avg["pm2.5atm"],
        marker="o",
        label="Hourly Average PM2.5",
        color=room_colour,
        alpha=0.8,
    )
    plt.axvspan(
        6, 12, color="orange", alpha=0.3, label="Morning Cooking + Lunch Window"
    )
    plt.axvspan(
        17, 22, color="orange", alpha=0.3, label="Evening Cooking + Dinner Window"
    )
    plt.title(f"Diurnal Variation of PM2.5 with Cooking Windows - {room_name}")
    plt.xlabel("Hour of Day")
    plt.ylabel("Average PM2.5 (µg/m³)")
    plt.xticks(range(0, 24))
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    # Save the plot
    os.makedirs(os.path.join(PLOT_DIR, "Cooking_Related_Analysis"), exist_ok=True)
    plt.savefig(
        os.path.join(
            PLOT_DIR,
            "Cooking_Related_Analysis",
            f"{room_name}_Diurnal_Cooking_Analysis.png",
        )
    )
    plt.show()


# --- Markdown cell 35 ---
# ## Decay Rate After Cooking (Ventilation Efficiency Proxy)
# - For each major spike Measure time taken for PM2.5 to return to baseline
# - We analyze:
#   - How long it takes to return to baseline
#   - How fast concentration decreases
# #### Steps:
# - For Each Cooking period, Identify Decay Start
#   - The time of maximum concentration within that period.
# - Define Baseline
#   - Average non-cooking period concentration
# - Calculate Decay Duration
#   - Start at period maximum concentration time
#   - Find first time when PM2.5 returns near baseline
# #### Plot:
# - Select one representative period (One with maximum duration or maximum concentration)
# - Plot concentration vs time for this period
# - Highlight:
#   - Peak maximum
#   - Recovery point
#   - Decay slope


# --- Markdown cell 36 ---
# ### Helper Function


# --- Code cell 37 ---
from datetime import time


def classify_period_simple(t):
    """
    Morning + Afternoon : 6:00 to 12:00 (From morning tea to Lunch)
    Evening : 17:00 to 22:00 (From evening tea to Dinner)
    Both classified as `Cooking`
    """
    if time(6, 0) <= t <= time(12, 0) or time(17, 0) <= t <= time(22, 0):
        return "Cooking"
    else:
        return "Non-Cooking"


def decay_rate_after_cooking(
    room_df=None, room_colour=None, room_name=None, room_base_avg=None
):
    if room_df is None:
        raise ValueError("room_df is None")
    if room_colour is None:
        raise ValueError("room_colour is None")
    if room_name is None:
        raise ValueError("room_name is None")
    if room_base_avg is None:
        raise ValueError("room_base_avg is None")

    room_decay_df = room_df[["pm2.5atm"]].copy()
    room_decay_df.index = pd.to_datetime(room_decay_df.index)

    room_decay_df["Date"] = room_decay_df.index.date
    room_decay_df["Time"] = room_decay_df.index.time
    room_decay_df["Period"] = room_decay_df["Time"].apply(classify_period_simple)

    room_daily_avg = room_decay_df.groupby("Date")["pm2.5atm"].mean()
    baseline_margin = 0.2

    room_decay_df["DailyAvg"] = room_decay_df["Date"].map(room_daily_avg)
    room_decay_df["Baseline"] = float(room_base_avg)

    is_major = (room_decay_df["Period"] == "Cooking") & (
        room_decay_df["pm2.5atm"] > room_decay_df["DailyAvg"]
    )
    start_of_event = is_major & ~is_major.shift(fill_value=False)
    room_decay_df["Event ID"] = start_of_event.cumsum()
    room_decay_df.loc[~is_major, "Event ID"] = np.nan

    records = []
    max_decay_hours = 12
    min_consecutive_points = 3
    min_decay_minutes = 10
    sustained_recovery_minutes = 15

    for event_id in room_decay_df["Event ID"].dropna().unique():
        event_data = room_decay_df[room_decay_df["Event ID"] == event_id].sort_index()
        if len(event_data) < 2:
            continue

        event_end_time = event_data.index.max()

        # Use the overall event maximum as the peak — this is the true start of decay,
        # not a tail-window anchor. The tail window approach was causing the peak to be
        # misidentified as the first point of the plot window.
        max_conc_time = event_data["pm2.5atm"].idxmax()
        max_conc_value = float(event_data["pm2.5atm"].max())
        max_conc_hour = max_conc_time.hour
        max_conc_date = max_conc_time.date()

        threshold = float(event_data["Baseline"].iloc[0]) * (1 + baseline_margin)

        search_window_end = max_conc_time + pd.Timedelta(hours=max_decay_hours)
        next_cooking_candidates = room_decay_df[
            (room_decay_df.index > event_end_time)
            & (room_decay_df["Period"] == "Cooking")
        ].index
        if len(next_cooking_candidates) > 0:
            next_cooking_start = next_cooking_candidates.min()
            recovery_search_end = min(search_window_end, next_cooking_start)
        else:
            recovery_search_end = search_window_end

        # Slice from the true peak onwards for recovery search
        post_peak_full = room_decay_df.loc[
            max_conc_time:recovery_search_end, "pm2.5atm"
        ]
        if len(post_peak_full) < min_consecutive_points:
            continue

        if len(post_peak_full) > 1:
            minutes_per_row = (
                post_peak_full.index[1] - post_peak_full.index[0]
            ).total_seconds() / 60.0
        else:
            minutes_per_row = 30.0
        sustained_points = max(1, round(sustained_recovery_minutes / minutes_per_row))

        below_threshold_mask = post_peak_full <= threshold
        sustained_below = (
            below_threshold_mask.rolling(
                window=sustained_points, min_periods=sustained_points
            ).sum()
            == sustained_points
        )
        recovery_candidates = sustained_below[sustained_below].index
        if len(recovery_candidates) == 0:
            continue
        window_end_idx = post_peak_full.index.get_loc(recovery_candidates.min())
        recovery_start_idx = max(0, window_end_idx - (sustained_points - 1))
        recovery_point = post_peak_full.index[recovery_start_idx]

        recovery_hour = recovery_point.hour
        decay_duration = (recovery_point - max_conc_time).total_seconds() / 3600.0

        if decay_duration <= 0 or decay_duration > max_decay_hours:
            continue
        if (decay_duration * 60) < min_decay_minutes:
            continue

        post_peak_data = post_peak_full.loc[max_conc_time:recovery_point]
        if len(post_peak_data) < min_consecutive_points:
            continue

        initial_conc = max_conc_value
        final_conc = float(post_peak_data.iloc[-1])
        decay_rate_per_hour = (initial_conc - final_conc) / decay_duration
        decay_rate_per_minute = decay_rate_per_hour / 60.0

        records.append(
            {
                "Event ID": event_id,
                "Date": max_conc_date,
                "Peak Time": max_conc_time,
                "Peak Hour": max_conc_hour,
                "Recovery Hour": recovery_hour,
                "Recovery Time": recovery_point,
                "Recovery Threshold": threshold,
                "Decay Duration (hours)": decay_duration,
                "Initial Concentration": initial_conc,
                "Final Concentration": final_conc,
                "Decay Rate (µg/m³/hour)": decay_rate_per_hour,
                "Decay Rate (µg/m³/minute)": decay_rate_per_minute,
            }
        )

    decay_df = pd.DataFrame(records)
    if decay_df.empty:
        print(f"\nDecay Rate Analysis After Cooking Events in {room_name}:")
        print("No valid decay events found.")
        return decay_df

    decay_df = decay_df.sort_values(by=["Date", "Peak Hour"]).reset_index(drop=True)

    print(f"\nDecay Rate Analysis After Cooking Events in {room_name}:")
    print(decay_df)
    os.makedirs(os.path.join(PLOT_DIR, "Decay_Rate_Analysis"), exist_ok=True)
    decay_df.to_csv(
        os.path.join(
            PLOT_DIR, "Decay_Rate_Analysis", f"{room_name}_Decay_Analysis.csv"
        ),
        index=False,
    )

    rep = decay_df.sort_values(
        by=["Initial Concentration", "Decay Duration (hours)"],
        ascending=[False, False],
    ).iloc[0]

    # Extract scalars from rep — no .loc lookups needed anywhere in plotting
    peak_time = pd.to_datetime(rep["Peak Time"])
    recovery_time = pd.to_datetime(rep["Recovery Time"])
    initial_concentration = float(rep["Initial Concentration"])
    final_concentration = float(rep["Final Concentration"])
    recovery_threshold = float(rep["Recovery Threshold"])
    decay_rate_per_hour = float(rep["Decay Rate (µg/m³/hour)"])
    decay_rate_per_minute = float(rep["Decay Rate (µg/m³/minute)"])
    peak_hour = int(rep["Peak Hour"])

    # Plot window: start slightly before peak so the rise context is visible,
    # end at recovery. This ensures peak_time sits inside the window, not at its edge.
    plot_start = peak_time - pd.Timedelta(minutes=30)
    event_data = room_decay_df.loc[plot_start:recovery_time].sort_index()
    baseline_concentration = float(event_data["Baseline"].iloc[0])

    plt.figure(figsize=(12, 6))
    plt.plot(
        event_data.index,
        event_data["pm2.5atm"],
        marker="o",
        label="PM2.5 Concentration",
        color=room_colour,
        alpha=0.8,
    )

    # Max Concentration line — drawn at the true peak value
    plt.axhline(
        y=initial_concentration,
        color="red",
        linestyle="-.",
        label=f"Max Concentration ({initial_concentration:.1f} µg/m³)",
        alpha=0.6,
    )
    plt.annotate(
        f"Peak {initial_concentration:.1f}",
        xy=(peak_time, initial_concentration),
        xytext=(3, 5),
        textcoords="offset points",
        arrowprops=dict(arrowstyle="->", color="red"),
        fontsize=9,
        color="red",
    )

    # Recovery threshold line
    plt.axhline(
        y=final_concentration,
        color="green",
        linestyle="-.",
        label=f"Recovery ({final_concentration:.1f} µg/m³)",
        alpha=0.6,
    )
    plt.annotate(
        f"Recovery {final_concentration:.1f}",
        xy=(recovery_time, final_concentration),
        xytext=(3, -15),
        textcoords="offset points",
        arrowprops=dict(arrowstyle="->", color="green"),
        fontsize=9,
        color="green",
    )

    # Baseline line
    plt.axhline(
        y=baseline_concentration,
        color="blue",
        linestyle="-.",
        label=f"Baseline ({baseline_concentration:.1f} µg/m³)",
        alpha=0.6,
    )
    plt.annotate(
        f"Baseline {baseline_concentration:.1f}",
        xy=(event_data.index[0], baseline_concentration),
        xytext=(3, -15),
        textcoords="offset points",
        arrowprops=dict(arrowstyle="->", color="blue"),
        fontsize=9,
        color="blue",
    )

    # Decay slope line — from true peak to recovery point, both as datetimes
    plt.plot(
        [peak_time, recovery_time],
        [initial_concentration, final_concentration],
        color="black",
        linestyle="--",
        linewidth=1.5,
        label=f"Decay Rate: {decay_rate_per_hour:.2f} µg/m³/hour, {decay_rate_per_minute:.2f} µg/m³/minute",
        alpha=0.7,
    )

    mid_time = peak_time + (recovery_time - peak_time) / 2
    mid_value = (initial_concentration + final_concentration) / 2
    plt.annotate(
        f"Decay Rate: {decay_rate_per_hour:.2f} µg/m³/hour\n{decay_rate_per_minute:.2f} µg/m³/minute",
        xy=(mid_time, mid_value),
        xytext=(-25, -50),
        textcoords="offset points",
        fontsize=9,
        fontweight="bold",
        color="black",
        rotation=-35,
    )

    plt.title(
        f"Decay Curve After Cooking Event on {rep['Date']} "
        f"(Peak Hour: {peak_hour}) "
        f"(Duration: {rep['Decay Duration (hours)']:.2f} hours, "
        f"Decay Rate: {decay_rate_per_hour:.2f} µg/m³/hour, {decay_rate_per_minute:.2f} µg/m³/minute)",
        fontweight="bold",
    )
    plt.xlabel("Time")
    plt.ylabel("PM2.5 Concentration (µg/m³)")
    plt.legend()
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Decay_Rate_Analysis"), exist_ok=True)
    plt.savefig(
        os.path.join(PLOT_DIR, "Decay_Rate_Analysis", f"{room_name}_Decay_Curve.png"),
        dpi=600,
        bbox_inches="tight",
    )
    plt.show()

    return decay_df


# --- Markdown cell 40 ---
# ## Spatial Gradient Plot
# - Compare mean PM2.5 across rooms
# - Compare peak intensity across rooms
# - Show attenuation (reduction %) from kitchen outward
# - Visualize gradient clearly
#
# #### Steps:
# - Combine All Room Data
#   - Add column `Room` to all 3 Kitchen, Hall, Bedroom
#   - Make sure that data is per minute to make sure that `DateTime` is common accroce all 3
# - Calculate Room-Wise Summary
#   - Average exposure
#   - Maximum spike
#   - Variability
# - Calculate Gradient Strength
# - Create Spatial Gradient Plot
#   - Filter only cooking periods
#   - Group them on column `Room` and average out `pm2.5atm`
#   - Plot this
# - Plot Time Synchronized Gradient
#   - Create Pivot table with index=`DateTime`, columns=`Room`, and value=`pm2.5atm `
#   - Then compute:
#     - kitchen - hall
#     - hall - bedroom


# --- Code cell 41 ---
def spatial_gradient_plot(kitchen_df=None, hall_df=None, bedroom_df=None):
    """
    Compare spatial PM2.5 gradients between rooms during cooking periods.

    Produces summary statistics, a bar plot during cooking windows, and a
    time-synchronized gradient plot.
    """
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Reset index to convert DateTime from index to column
    bedroom_df = bedroom_df.reset_index().copy()
    hall_df = hall_df.reset_index().copy()
    kitchen_df = kitchen_df.reset_index().copy()

    # Keep only time and PM2.5 columns for gradient calculations.
    bedroom_df_gradient = bedroom_df[["DateTime", "pm2.5atm"]].copy()
    hall_df_gradient = hall_df[["DateTime", "pm2.5atm"]].copy()
    kitchen_df_gradient = kitchen_df[["DateTime", "pm2.5atm"]].copy()
    # Remove outliers to avoid skewing gradients.
    bedroom_df_gradient = remove_outliers_iqr(bedroom_df_gradient, "pm2.5atm", 200)
    hall_df_gradient = remove_outliers_iqr(hall_df_gradient, "pm2.5atm", 1500)
    kitchen_df_gradient = remove_outliers_iqr(kitchen_df_gradient, "pm2.5atm", 1500)
    # Convert to minute-level data to align sensors.
    bedroom_df_gradient["DateTime"] = bedroom_df_gradient["DateTime"].dt.floor("min")
    hall_df_gradient["DateTime"] = hall_df_gradient["DateTime"].dt.floor("min")
    kitchen_df_gradient["DateTime"] = kitchen_df_gradient["DateTime"].dt.floor("min")
    # Group by DateTime and take mean.
    bedroom_df_gradient = (
        bedroom_df_gradient.groupby(bedroom_df_gradient["DateTime"], as_index=False)
        .mean()
        .reset_index()
    )
    hall_df_gradient = (
        hall_df_gradient.groupby(hall_df_gradient["DateTime"], as_index=False)
        .mean()
        .reset_index()
    )
    kitchen_df_gradient = (
        kitchen_df_gradient.groupby(kitchen_df_gradient["DateTime"], as_index=False)
        .mean()
        .reset_index()
    )
    # Add Room column for combined plotting.
    bedroom_df_gradient["Room"] = "Bedroom"
    hall_df_gradient["Room"] = "Hall"
    kitchen_df_gradient["Room"] = "Kitchen"
    # Combine all three dataframes
    combined_df = pd.concat(
        [kitchen_df_gradient, hall_df_gradient, bedroom_df_gradient]
    )

    # Room-wise summary statistics.
    room_summary = combined_df.groupby("Room")["pm2.5atm"].agg(
        ["mean", "max", "min", "std"]
    )
    print("Room-wise Summary of PM2.5 Levels:")
    print(room_summary)

    # Calculate gradient strength (kitchen vs other rooms).
    kitchen_mean = room_summary.loc["Kitchen", "mean"]
    hall_mean = room_summary.loc["Hall", "mean"]
    bedroom_mean = room_summary.loc["Bedroom", "mean"]
    gradient_kh = ((kitchen_mean - hall_mean) / kitchen_mean) * 100  # in percentage
    gradient_kb = ((kitchen_mean - bedroom_mean) / kitchen_mean) * 100  # in percentage
    print(f"\nGradient strength reduction from Kitchen to Hall: {gradient_kh:.2f}%")
    print(f"Gradient strength reduction from Kitchen to Bedroom: {gradient_kb:.2f}%")

    # Create spatial gradient plot during cooking only.
    # Classify periods as cooking or non-cooking.
    combined_df["Period"] = combined_df["DateTime"].dt.time.apply(
        classify_period_simple
    )
    combined_df = combined_df[combined_df["Period"] != "Non-Cooking"]
    # Group by room and take mean.
    spatial_gradient_df = combined_df.groupby(["Room"])["pm2.5atm"].mean().reset_index()
    # Plot this
    plt.figure(figsize=(8, 5))
    bars = plt.bar(
        spatial_gradient_df["Room"],
        spatial_gradient_df["pm2.5atm"],
        color=["red", "orange", "yellow"],
    )
    plt.title("Spatial Gradient of PM2.5 During Cooking Periods")
    plt.xlabel("Room")
    plt.ylabel("Average PM2.5 (ug/m3)")
    # Add value labels on bars
    for bar in bars:
        height = bar.get_height()
        plt.text(
            bar.get_x() + bar.get_width() / 2.0,
            height,
            f"{height:.1f}",
            ha="center",
            va="bottom",
        )
    plt.tight_layout()
    # Save plot
    os.makedirs(f"{PLOT_DIR}/Spatial_Gradient_Plot", exist_ok=True)
    plt.savefig(
        f"{PLOT_DIR}/Spatial_Gradient_Plot/Spatial_Gradient_During_Cooking.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()

    # Plot Time Synchronized Gradient
    pivot_df = combined_df.pivot_table(
        index="DateTime", columns="Room", values="pm2.5atm"
    )
    # Compute differences
    pivot_df["Kitchen-Hall"] = pivot_df["Kitchen"] - pivot_df["Hall"]
    pivot_df["Hall-Bedroom"] = pivot_df["Hall"] - pivot_df["Bedroom"]
    # Plot this diffrence over time
    plt.figure(figsize=(15, 5))
    plt.plot(pivot_df.index, pivot_df["Kitchen-Hall"], label="Kitchen-Hall", marker="o")
    plt.plot(pivot_df.index, pivot_df["Hall-Bedroom"], label="Hall-Bedroom", marker="s")
    plt.title("Temporal Gradient of PM2.5 Between Rooms")
    plt.xlabel("Time")
    plt.ylabel("Difference in PM2.5 (ug/m3)")
    plt.legend()
    plt.tight_layout()
    # Save plot
    os.makedirs(f"{PLOT_DIR}/Spatial_Gradient_Plot", exist_ok=True)
    plt.savefig(
        f"{PLOT_DIR}/Spatial_Gradient_Plot/Temporal_Gradient.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()


# --- Markdown cell 43 ---
# ## Particle Size Distribution (Source Characterization)
# - Using : `c_300`, `c_500`, `c_1000`, `c_2500`, `c_5000`, `c_10000`
# - Compare mean counts across rooms
# - Fine particles dominance => combustion source
# - Coarse particles dominance => dust/resuspension


# --- Code cell 44 ---
def particle_size_distribution(kitchen_df=None, hall_df=None, bedroom_df=None):
    """
    Summarize particle count distributions by size and room.

    Outputs mean counts for different particle size bins across rooms.
    """
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Select particle count channels for each room.
    kitchen_df_particals = kitchen_df[
        ["c_300", "c_500", "c_1000", "c_2500", "c_5000", "c_10000"]
    ].copy()
    hall_df_particals = hall_df[
        ["c_300", "c_500", "c_1000", "c_2500", "c_5000", "c_10000"]
    ].copy()
    bedroom_df_particals = bedroom_df[
        ["c_300", "c_500", "c_1000", "c_2500", "c_5000", "c_10000"]
    ].copy()

    # Add room column
    kitchen_df_particals["Room"] = "Kitchen"
    hall_df_particals["Room"] = "Hall"
    bedroom_df_particals["Room"] = "Bedroom"

    # Combine all three dataframes
    combined_df_particals = pd.concat(
        [kitchen_df_particals, hall_df_particals, bedroom_df_particals]
    )

    # Compare mean counts across all room
    particle_summary = combined_df_particals.groupby("Room").mean()
    print("Mean Particle Counts Across Rooms:")
    print(particle_summary)


# --- Markdown cell 46 ---
# ## Correlation heatmap for each room


# --- Code cell 47 ---
def correlation_heatmap(kitchen_df=None, hall_df=None, bedroom_df=None):
    """
    Plot correlation heatmaps for each room.

    Generates full-variable heatmaps and PM/count-only heatmaps, then saves
    each image to the correlation plots directory.
    """
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Create heatmap for kitchen (all variables).
    kitchen_corr = kitchen_df.corr()
    plt.figure(figsize=(10, 8))
    sns.heatmap(kitchen_corr, annot=True, cmap="coolwarm", center=0)
    plt.title("Kitchen Correlation Heatmap")
    plt.tight_layout()
    os.makedirs(f"{PLOT_DIR}/Correlation_Heatmap", exist_ok=True)
    plt.savefig(
        f"{PLOT_DIR}/Correlation_Heatmap/Kitchen_Correlation_Heatmap.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()

    # Create heatmap for kitchen for only PMs and particle counts.
    kitchen_pms_counts_corr = (
        kitchen_df[
            [
                "PM1",
                "PM2.5",
                "PM10",
                "pm1atm",
                "pm2.5atm",
                "pm10atm",
                "c_300",
                "c_500",
                "c_1000",
                "c_2500",
                "c_5000",
                "c_10000",
            ]
        ]
        .copy()
        .corr()
    )
    plt.figure(figsize=(10, 8))
    sns.heatmap(kitchen_pms_counts_corr, annot=True, cmap="coolwarm", center=0)
    plt.title("Kitchen Pms and counts Correlation Heatmap")
    plt.tight_layout()
    plt.savefig(
        f"{PLOT_DIR}/Correlation_Heatmap/Kitchen_pms_counts_Correlation_Heatmap.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()

    # Create heatmap for hall (all variables).
    hall_corr = hall_df.corr()
    plt.figure(figsize=(10, 8))
    sns.heatmap(hall_corr, annot=True, cmap="coolwarm", center=0)
    plt.title("Hall Correlation Heatmap")
    plt.tight_layout()
    plt.savefig(
        f"{PLOT_DIR}/Correlation_Heatmap/Hall_Correlation_Heatmap.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()
    # Create heatmap for hall for only PMs and particle counts.
    hall_pms_counts_corr = (
        hall_df[
            [
                "PM1",
                "PM2.5",
                "PM10",
                "pm1atm",
                "pm2.5atm",
                "pm10atm",
                "c_300",
                "c_500",
                "c_1000",
                "c_2500",
                "c_5000",
                "c_10000",
            ]
        ]
        .copy()
        .corr()
    )
    plt.figure(figsize=(10, 8))
    sns.heatmap(hall_pms_counts_corr, annot=True, cmap="coolwarm", center=0)
    plt.title("Hall Pms and counts Correlation Heatmap")
    plt.tight_layout()
    plt.savefig(
        f"{PLOT_DIR}/Correlation_Heatmap/Hall_pms_counts_Correlation_Heatmap.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()

    # Create heatmap for bedroom (all variables).
    bedroom_corr = bedroom_df.corr()
    plt.figure(figsize=(10, 8))
    sns.heatmap(bedroom_corr, annot=True, cmap="coolwarm", center=0)
    plt.title("Bedroom Correlation Heatmap")
    plt.tight_layout()
    plt.savefig(
        f"{PLOT_DIR}/Correlation_Heatmap/Bedroom_Correlation_Heatmap.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()

    # Create heatmap for bedroom for only PMs and particle counts.
    bedroom_pms_counts_corr = (
        bedroom_df[
            [
                "PM1",
                "PM2.5",
                "PM10",
                "pm1atm",
                "pm2.5atm",
                "pm10atm",
                "c_300",
                "c_500",
                "c_1000",
                "c_2500",
                "c_5000",
                "c_10000",
            ]
        ]
        .copy()
        .corr()
    )
    plt.figure(figsize=(10, 8))
    sns.heatmap(bedroom_pms_counts_corr, annot=True, cmap="coolwarm", center=0)
    plt.title("Bedrrom Pms and counts Correlation Heatmap")
    plt.tight_layout()
    plt.savefig(
        f"{PLOT_DIR}/Correlation_Heatmap/Bedroom_pms_counts_Correlation_Heatmap.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()


def frequency_analysis(kitchen_df=None, hall_df=None, bedroom_df=None):
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")

    # Round off pm2.5atm to nearest integer for frequency counts
    bedroom_vals = bedroom_df["pm2.5atm"].round().dropna().astype(int)
    hall_vals = hall_df["pm2.5atm"].round().dropna().astype(int)
    kitchen_vals = kitchen_df["pm2.5atm"].round().dropna().astype(int)

    # Count occurrences of each pm2.5atm value
    bedroom_freq = bedroom_vals.value_counts().sort_index()
    hall_freq = hall_vals.value_counts().sort_index()
    kitchen_freq = kitchen_vals.value_counts().sort_index()

    # Align all pm2.5atm bins across rooms for plotting
    freq_index = bedroom_freq.index.union(hall_freq.index).union(kitchen_freq.index)
    freq_index = (
        pd.Index(pd.to_numeric(freq_index, errors="coerce")).dropna().sort_values()
    )
    freq_df = pd.DataFrame(
        {
            "pm2.5atm": freq_index,
            "Bedroom": bedroom_freq.reindex(freq_index, fill_value=0).values,
            "Hall": hall_freq.reindex(freq_index, fill_value=0).values,
            "Kitchen": kitchen_freq.reindex(freq_index, fill_value=0).values,
        }
    )

    # Plot the frequency distribution for each room with shaded area
    x = freq_df["pm2.5atm"].to_numpy(dtype=float)
    kitchen_y = freq_df["Kitchen"].to_numpy(dtype=float)
    hall_y = freq_df["Hall"].to_numpy(dtype=float)
    bedroom_y = freq_df["Bedroom"].to_numpy(dtype=float)

    plt.figure(figsize=(15, 8))
    plt.plot(x, kitchen_y, color=kitchen_colour, label="Kitchen", alpha=0.9)
    plt.fill_between(x, kitchen_y, alpha=0.3, color=kitchen_colour)
    plt.plot(x, hall_y, color=hall_colour, label="Hall", alpha=0.9)
    plt.fill_between(x, hall_y, alpha=0.3, color=hall_colour)
    plt.plot(x, bedroom_y, color=bedroom_colour, label="Bedroom", alpha=0.9)
    plt.fill_between(x, bedroom_y, alpha=0.3, color=bedroom_colour)
    plt.xlabel("PM2.5 (µg/m³)")
    plt.ylabel("Frequency (Number of Occurrences)")

    # Set axis limits and show x-axis ticks every 50 units
    x_min, x_max = 0, int(max(freq_df["pm2.5atm"].max() * 1.1, 100))
    plt.xlim(x_min, x_max)
    plt.xticks(range(0, x_max + 1, 50))

    # Set y-axis limits with a little padding
    y_max = max(freq_df[["Kitchen", "Hall", "Bedroom"]].max()) * 1.1
    plt.ylim(0, y_max)

    plt.title("Frequency Distribution of PM2.5")
    plt.legend()
    plt.grid(alpha=0.3)
    # Save the plot
    plt.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Frequency_Analysis"), exist_ok=True)
    plt.savefig(
        os.path.join(PLOT_DIR, "Frequency_Analysis", "Frequency_Distribution.png")
    )
    # Show the plot
    plt.show()

    # For each room, calculate the percentage of time pm2.5atm was in the following bins: 0-15, 15-60, >60
    bins = [0, 15, 60, np.inf]
    labels = ["0-15", "15-60", ">60"]
    kitchen_bins = pd.cut(kitchen_vals, bins=bins, labels=labels, right=False)
    hall_bins = pd.cut(hall_vals, bins=bins, labels=labels, right=False)
    bedroom_bins = pd.cut(bedroom_vals, bins=bins, labels=labels, right=False)
    kitchen_bin_counts = kitchen_bins.value_counts(normalize=True) * 100
    hall_bin_counts = hall_bins.value_counts(normalize=True) * 100
    bedroom_bin_counts = bedroom_bins.value_counts(normalize=True) * 100
    print("Percentage of time PM2.5 was in the following bins:")
    print("Kitchen:")
    print(kitchen_bin_counts)
    print("Hall:")
    print(hall_bin_counts)
    print("Bedroom:")
    print(bedroom_bin_counts)


# ============================================================================
# 24-Hour Average Analysis (WHO & NAAQS Guidelines Comparison)
# ============================================================================
def daily_average_guideline_comparison(kitchen_df=None, hall_df=None, bedroom_df=None):
    if kitchen_df is None:
        raise ValueError("kitchen_df is None")
    if hall_df is None:
        raise ValueError("hall_df is None")
    if bedroom_df is None:
        raise ValueError("bedroom_df is None")

    # Calculate 24-hour daily averages for each room
    kitchen_daily_avg = kitchen_df.resample("D")["pm2.5atm"].mean()
    hall_daily_avg = hall_df.resample("D")["pm2.5atm"].mean()
    bedroom_daily_avg = bedroom_df.resample("D")["pm2.5atm"].mean()

    # WHO 24hr Guideline: 15 µg/m³
    # India 24hr NAAQS: 60 µg/m³
    who_guideline = 15
    naaqs_guideline = 60

    # Calculate percentage of days exceeding each guideline
    kitchen_who_pct = (kitchen_daily_avg > who_guideline).mean() * 100
    kitchen_naaqs_pct = (kitchen_daily_avg > naaqs_guideline).mean() * 100

    hall_who_pct = (hall_daily_avg > who_guideline).mean() * 100
    hall_naaqs_pct = (hall_daily_avg > naaqs_guideline).mean() * 100

    bedroom_who_pct = (bedroom_daily_avg > who_guideline).mean() * 100
    bedroom_naaqs_pct = (bedroom_daily_avg > naaqs_guideline).mean() * 100

    # Print results
    print("\n" + "=" * 80)
    print("24-Hour Average Daily Analysis - WHO & NAAQS Guidelines Comparison")
    print("=" * 80)
    print("\nDaily Average Statistics (µg/m³):")
    print(f"\nKitchen:")
    print(f"  Mean Daily Average: {kitchen_daily_avg.mean():.2f}")
    print(f"  Min Daily Average: {kitchen_daily_avg.min():.2f}")
    print(f"  Max Daily Average: {kitchen_daily_avg.max():.2f}")
    print(f"  Std Dev: {kitchen_daily_avg.std():.2f}")
    print(f"  % Days > WHO Guideline (15 µg/m³): {kitchen_who_pct:.2f}%")
    print(f"  % Days > NAAQS Guideline (60 µg/m³): {kitchen_naaqs_pct:.2f}%")

    print(f"\nHall:")
    print(f"  Mean Daily Average: {hall_daily_avg.mean():.2f}")
    print(f"  Min Daily Average: {hall_daily_avg.min():.2f}")
    print(f"  Max Daily Average: {hall_daily_avg.max():.2f}")
    print(f"  Std Dev: {hall_daily_avg.std():.2f}")
    print(f"  % Days > WHO Guideline (15 µg/m³): {hall_who_pct:.2f}%")
    print(f"  % Days > NAAQS Guideline (60 µg/m³): {hall_naaqs_pct:.2f}%")

    print(f"\nBedroom:")
    print(f"  Mean Daily Average: {bedroom_daily_avg.mean():.2f}")
    print(f"  Min Daily Average: {bedroom_daily_avg.min():.2f}")
    print(f"  Max Daily Average: {bedroom_daily_avg.max():.2f}")
    print(f"  Std Dev: {bedroom_daily_avg.std():.2f}")
    print(f"  % Days > WHO Guideline (15 µg/m³): {bedroom_who_pct:.2f}%")
    print(f"  % Days > NAAQS Guideline (60 µg/m³): {bedroom_naaqs_pct:.2f}%")

    # Create comparison plot
    fig, ax = plt.subplots(figsize=(14, 8))

    rooms = ["Kitchen", "Hall", "Bedroom"]
    who_percentages = [kitchen_who_pct, hall_who_pct, bedroom_who_pct]
    naaqs_percentages = [kitchen_naaqs_pct, hall_naaqs_pct, bedroom_naaqs_pct]
    room_colors_list = [kitchen_colour, hall_colour, bedroom_colour]

    x = np.arange(len(rooms))
    width = 0.35

    bars1 = ax.bar(
        x - width / 2,
        who_percentages,
        width,
        label="WHO Guideline (15 µg/m³)",
        alpha=0.8,
        color="#984EA3",
    )
    bars2 = ax.bar(
        x + width / 2,
        naaqs_percentages,
        width,
        label="NAAQS Guideline (60 µg/m³)",
        alpha=0.8,
        color="#A65628",
    )

    ax.set_xlabel("Room", fontsize=14, fontweight="bold")
    ax.set_ylabel("% of Days Exceeding Guideline", fontsize=14, fontweight="bold")
    ax.set_title(
        "Daily Average PM2.5: % Days Exceeding WHO & NAAQS Guidelines",
        fontsize=16,
        fontweight="bold",
    )
    ax.set_xticks(x)
    ax.set_xticklabels(rooms, fontsize=12)
    ax.legend(fontsize=12)
    ax.tick_params(axis="y", labelsize=12)
    ax.grid(axis="y", alpha=0.3)

    # Add value labels on bars
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                height,
                f"{height:.1f}%",
                ha="center",
                va="bottom",
                fontsize=11,
                fontweight="bold",
            )

    fig.tight_layout()
    os.makedirs(os.path.join(PLOT_DIR, "Daily_Average_Analysis"), exist_ok=True)
    fig.savefig(
        os.path.join(
            PLOT_DIR, "Daily_Average_Analysis", "Daily_Average_Guideline_Comparison.png"
        ),
        dpi=300,
        bbox_inches="tight",
    )
    plt.show()

    # Create a summary dataframe and save to CSV
    summary_df = pd.DataFrame(
        {
            "Room": rooms,
            "Mean Daily Avg (µg/m³)": [
                kitchen_daily_avg.mean(),
                hall_daily_avg.mean(),
                bedroom_daily_avg.mean(),
            ],
            "Min Daily Avg (µg/m³)": [
                kitchen_daily_avg.min(),
                hall_daily_avg.min(),
                bedroom_daily_avg.min(),
            ],
            "Max Daily Avg (µg/m³)": [
                kitchen_daily_avg.max(),
                hall_daily_avg.max(),
                bedroom_daily_avg.max(),
            ],
            "Std Dev (µg/m³)": [
                kitchen_daily_avg.std(),
                hall_daily_avg.std(),
                bedroom_daily_avg.std(),
            ],
            "% Days > WHO (15 µg/m³)": who_percentages,
            "% Days > NAAQS (60 µg/m³)": naaqs_percentages,
        }
    )

    summary_df.to_csv(
        os.path.join(
            PLOT_DIR, "Daily_Average_Analysis", "Daily_Average_Guideline_Summary.csv"
        ),
        index=False,
    )
    print("\nSummary saved to: Daily_Average_Guideline_Summary.csv")


if __name__ == "__main__":
    main()
