import pandas as pd


def load_data_file(file_path):
    """
    Load data from a CSV file.

    Parameters:
    file_path (str): The path to the CSV file.

    Returns:
    pd.DataFrame: A DataFrame containing the loaded data.
    """
    try:
        data = pd.read_csv(file_path)
        print(f"Data loaded successfully from {file_path}")
        return data
    except Exception as e:
        print(f"An error occurred while loading data: {e}")
        return None


def load_data_directory(directory_path):
    """
    Load data from all CSV files in a specified directory.

    Parameters:
    directory_path (str): The path to the directory containing CSV files.

    Returns:
    dict: A dictionary where keys are file names and values are DataFrames of the loaded data.
    """
    import os

    data_dict = {}

    try:
        for file_name in os.listdir(directory_path):
            if file_name.endswith(".csv"):
                file_path = os.path.join(directory_path, file_name)
                data_dict[file_name] = load_data_file(file_path)
        print(f"Data loaded successfully from directory: {directory_path}")
        return data_dict
    except Exception as e:
        print(f"An error occurred while loading data from directory: {e}")
        return None


if __name__ == "__main__":
    # Example usage
    file_path = f"data\\Dhruv_Patel\\Bedroom\\ACRL_005_AU_PMS_CAPSTONE_2025-12-26.csv"  # Replace with your actual file path
    data = load_data_file(file_path)
    if data is not None:
        print(data.head())  # Print the first few rows of the loaded data
        print(data.info())  # Print information about the loaded data

    # Example usage for loading data from a directory
    directory_path = (
        f"data\\Dhruv_Patel\\Bedroom"  # Replace with your actual directory path
    )
    data_dict = load_data_directory(directory_path)
    if data_dict is not None:
        for file_name, df in data_dict.items():
            print(f"Data from {file_name}:")
            print(df.head())  # Print the first few rows of each loaded DataFrame
            print(df.info())  # Print information about each loaded DataFrame
