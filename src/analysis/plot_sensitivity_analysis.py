import numpy as np
import pandas as pd
import argparse
import matplotlib.pyplot as plt
import os

from experiments_to_dataframe import files_to_dataframe

# The result column used for the sensitivity analysis
col = 'ts_hospitalized'

# Compute amplitude of the hospital admissions wave
def comp_amplitude(row):
  array = row[col]
  max_value = np.max(array)
  return max_value

# Compute the error on the amplitude of the hospital admissions wave
def comp_amplitude_error(row):
  array = row[col]
  max_index = np.argmax(array)
  return row['ts_hospitalized_error_low'][max_index]

# Compute full width at half max of the hospital admissions wave
def comp_fwhm(row):
  array = row[col]
  max_value = np.max(array)
  half_max = 0.5 * max_value

  # Find indices where array values are greater than or equal to half of the maximum value
  above_half_max = array >= half_max

  # Find the first and last indices satisfying the condition
  first_index = np.argmax(above_half_max)
  last_index = len(array) - 1 - np.argmax(above_half_max[::-1])

  # Calculate FWHM as the difference between last and first indices
  fwhm = last_index - first_index

  return fwhm

# Compute the error on the full width at half max of the hospital admissions wave
def comp_fwhm_error(row):
  array = row[col]
  max_value = np.max(array)
  half_max = 0.5 * max_value

  # Find indices where array values are greater than or equal to half of the maximum value
  above_half_max = array >= half_max

  # Find the first and last indices satisfying the condition
  first_index = np.argmax(above_half_max)
  last_index = len(array) - 1 - np.argmax(above_half_max[::-1])

  return (row['ts_hospitalized_error_low'][first_index] + row['ts_hospitalized_error_low'][last_index] ) / 2

def comp_nominal_val(parameter, projectdir):
  real_datapath = os.path.join(projectdir, 'src/data/observed_hospital_admissions_per_day.csv')
  real_hospitalized = pd.read_csv(real_datapath)
  real_hospitalized.loc[-1] = [0]  # adding a row
  real_hospitalized.index = real_hospitalized.index + 1  # shifting index
  real_hospitalized.sort_index(inplace=True) 
  real_hospitalized.drop(real_hospitalized.tail(1).index,inplace=True) # drop last n rows
  real_hospitalized = real_hospitalized.iloc[:, 0].tolist()
  return 0.07

def generate_plot(xparam, nominal_val, projectdir):
  # plt.plot(df[xparam], df[yparam], marker='o', linestyle="", color='black')
  fig, axs = plt.subplots(1, 2, figsize=(15, 6))
  axs[0].errorbar(df[xparam], df['fwhm'], yerr=df['fwhm_error'], fmt='o', capsize=3, capthick=1, color='black')
  axs[0].set_xlabel(xparam)
  axs[0].set_ylabel('fwhm')
  axs[0].axvline(x=nominal_val, linestyle='--', color='grey', label='Nominal value')
  axs[0].legend(loc='upper right')

  axs[1].errorbar(df[xparam], df['amplitude'], yerr=df['amplitude_error'], fmt='o', capsize=3, capthick=1, color='black')
  axs[1].set_xlabel(xparam)
  axs[1].set_ylabel('amplitude')
  axs[1].axvline(x=nominal_val, linestyle='--', color='grey', label='Nominal value')
  axs[1].legend(loc='upper right')

  fig.suptitle(f"Plot of {xparam} against amplitude and FWHM")
  # plt.show()
  figure_dir = os.path.join(projectdir, 'build/output/figures')
  if not os.path.isdir(figure_dir):
    os.makedirs(figure_dir)
  plt.savefig(os.path.join(figure_dir, 'sensitivity_analysis.png'))

# Create arg parser
parser = argparse.ArgumentParser()
# e.g. --experiment=experiments_2023-10-05_15-01-54
parser.add_argument('--experiment', type=str, required=True)
parser.add_argument('--projectdir', type=str, required=True)
parser.add_argument('--parameter', type=str, default='beta1')
args = parser.parse_args()

output_dir = os.path.join(args.projectdir, 'build/output/')
df = files_to_dataframe(output_dir, args.experiment)

df['amplitude'] = df.apply(comp_amplitude, axis=1)
df['amplitude_error'] = df.apply(comp_amplitude_error, axis=1)
df['fwhm'] = df.apply(comp_fwhm, axis=1)
df['fwhm_error'] = df.apply(comp_fwhm_error, axis=1)

nominal_val = comp_nominal_val(args.parameter, args.projectdir)

# nominal_row = df[df[parameter] == nominal_val]
# df['amplitude'] = df['amplitude'] / nominal_row['amplitude'].values[0] - 1
# df['amplitude_error'] = df['amplitude_error'] / nominal_row['amplitude'].values[0] - 1
# df['fwhm'] = df['fwhm'] / nominal_row['fwhm'].values[0] - 1

generate_plot(args.parameter, nominal_val, args.projectdir)
