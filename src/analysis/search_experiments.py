import os
import argparse
import json
import datetime

from pprint import pprint

parser = argparse.ArgumentParser()
parser.add_argument('--from-files', type=str, required=True)
args = parser.parse_args()

experiment_dirs = [d for d in os.listdir(args.from_files) if d.startswith('experiments_')]

experiment_parameters = {}
for experiment_dir in experiment_dirs:
  full_dirpath = os.path.join(args.from_files, experiment_dir)
  experiment_files = os.listdir(full_dirpath)
  if len(experiment_files) != 0:
    # Use os.path.join() to create full paths for each item
    full_paths = [os.path.join(full_dirpath, file) for file in experiment_files]
    # Use os.path.isdir() to filter only directories
    subdirectories = [path for path in full_paths if os.path.isdir(path)]
    params_file_path = os.path.join(subdirectories[0], 'param.json')
    with open(params_file_path, 'r') as params_file:
      parameters = json.load(params_file)
    experiment_parameters[experiment_dir] = parameters.get("bdm::OptimizationParam", {})

pprint(experiment_parameters)
