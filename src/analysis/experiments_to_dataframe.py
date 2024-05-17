import os
import sys
import datetime
import json
import pandas as pd
from collections import defaultdict

param_filter = ['root_style', '_typename']

def files_to_dataframe(base_dir, experiment):
    if not os.path.exists(base_dir):
        print(f"Directory {base_dir} does not exist.")
        sys.exit(1)

    # Initialize defaultdict
    data = defaultdict(list)

    # Read the directory containing the experiments
    if experiment:
        experiment_dirs = [experiment]
    else:
        experiment_dirs = [d for d in os.listdir(base_dir) if d.startswith('experiments_')]

    for experiment_dir in experiment_dirs:
        timestamp = experiment_dir.split("experiments_")[1]
        timestamp = datetime.datetime.strptime(timestamp, '%Y-%m-%d_%H-%M-%S')
        experiment_path = os.path.join(base_dir, experiment_dir)
        uuid_dirs = os.listdir(experiment_path)

        for uuid_dir in uuid_dirs:
            uuid_path = os.path.join(experiment_path, uuid_dir)
            csv_files = [f for f in os.listdir(uuid_path) if f.endswith('.csv')]
            mse_file = [f for f in os.listdir(uuid_path) if f.endswith('.json')][0]

            data_files = []
            for csv_file in csv_files:
                if csv_file.startswith('ts_'):
                    data_files.append(csv_file)

            if mse_file:
                mse_path = os.path.join(uuid_path, mse_file)
                with open(mse_path) as json_file:
                    # Load the JSON data into a Python object
                    params = json.load(json_file)
                    # Remove uninteresting parameters to avoid clutter
                    for f in param_filter:
                        del params['bdm::SimParam'][f]

                data['uuid'].append(uuid_dir)
                data['run_at'].append(timestamp)
                data['resolution'].append(params['resolution'])
                data['repetitions'].append(params['repetitions'])
                data['mse'].append(params['mse'])
                for k, v in params['bdm::SimParam'].items():
                    data[k].append(v)

            for data_file in data_files:
                data_path = os.path.join(uuid_path, data_file)
                df = pd.read_csv(data_path)
                y_values = df['y'].tolist()
                if "y_error_low" in df.columns and "y_error_high" in df.columns:
                    y_error_low = df['y_error_low'].tolist()
                    y_error_high = df['y_error_high'].tolist()
                    column_name_yl = data_file.replace('.csv', '') + "_error_low"
                    column_name_yh = data_file.replace('.csv', '') + "_error_high"
                    # print(column_name_yh)
                    data[column_name_yl].append(y_error_low)
                    data[column_name_yh].append(y_error_high)
                column_name = data_file.replace('.csv', '')
                data[column_name].append(y_values)

    # Create a pandas dataframe from the dictionary
    df_all = pd.DataFrame(data)

    return df_all

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python experiments_to_dataframe.py base_dir")
        sys.exit(1)

    base_dir = sys.argv[1]
    df = files_to_dataframe(base_dir)
    print(df)
