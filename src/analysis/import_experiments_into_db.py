#!/usr/bin/env python3.9

import os
import sys
import psycopg2
import pandas as pd
import datetime
import numpy as np
from psycopg2.extensions import register_adapter, AsIs

def adapt_numpy_int64(numpy_int64):
    return AsIs(numpy_int64)
register_adapter(np.int64, adapt_numpy_int64)

if len(sys.argv) != 2:
    print("Usage: python script_name.py base_dir")
    sys.exit(1)

base_dir = sys.argv[1]

if not os.path.exists(base_dir):
    print(f"Directory {base_dir} does not exist.")
    sys.exit(1)

# Connect to the database
conn = psycopg2.connect(dbname='bdm', user='ahmadh')

# Create a cursor object
cur = conn.cursor()

# Create the table to store the data
cur.execute("""
CREATE TABLE IF NOT EXISTS experiments (
    uuid TEXT PRIMARY KEY,
    run_at TIMESTAMP,
    resolution REAL,
    repetitions INT,
    mse REAL,
    beta1 REAL,
    beta2 REAL,
    beta3 REAL,
    beta4 REAL
);
""")

conn.commit()

# Read the directory containing the experiments
experiment_dirs = [d for d in os.listdir(base_dir) if d.startswith('experiments_')]

for experiment_dir in experiment_dirs:
    timestamp = experiment_dir.split("experiments_")[1]
    timestamp = datetime.datetime.strptime(timestamp, '%Y-%m-%d_%H-%M-%S')
    experiment_path = os.path.join(base_dir, experiment_dir)
    uuid_dirs = os.listdir(experiment_path)

    for uuid_dir in uuid_dirs:
        uuid_path = os.path.join(experiment_path, uuid_dir)
        csv_files = [f for f in os.listdir(uuid_path) if f.endswith('.csv')]

        mse_file = None
        data_files = []
        for csv_file in csv_files:
            if csv_file.startswith('params_and_error'):
                mse_file = csv_file
            elif csv_file.startswith('ts_'):
                data_files.append(csv_file)

        if mse_file:
            mse_path = os.path.join(uuid_path, mse_file)
            df = pd.read_csv(mse_path)

            uuid = uuid_dir
            repetitions = df.at[0, 'repetitions'].astype(int)
            resolution = df.at[0, 'resolution'].astype(float)
            mse = df.at[0, 'mse'].astype(float)
            beta1 = df.at[0, 'beta1'].astype(float)
            beta2 = df.at[0, 'beta2'].astype(float)
            beta3 = df.at[0, 'beta3'].astype(float)
            beta4 = df.at[0, 'beta4'].astype(float)

            cur.execute("""
            INSERT INTO experiments (uuid, run_at, resolution, repetitions, mse, beta1, beta2, beta3, beta4)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
            ON CONFLICT (uuid) DO NOTHING;
            """, (uuid, timestamp, resolution, repetitions, mse, beta1, beta2, beta3, beta4))

        for data_file in data_files:
            data_path = os.path.join(uuid_path, data_file)
            df = pd.read_csv(data_path)
            y_values = df['y'].tolist()
            column_name = data_file.replace('.csv', '')
            # cur.execute("ALTER TABLE experiments ADD COLUMN IF NOT EXISTS %s real[]" % column_name)
            cur.execute("UPDATE experiments SET %s = ARRAY%s WHERE uuid = '%s'" % (column_name, y_values, uuid))

conn.commit()
cur.close()
conn.close()

print("Data import completed successfully.")
