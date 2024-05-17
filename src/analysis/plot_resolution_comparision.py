import argparse, os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

from experiments_to_dataframe import files_to_dataframe

color_palette = ['#e66101','#fdb863','#b2abd2','#5e3c99']

# Create arg parser
parser = argparse.ArgumentParser()
# e.g. --experiment=experiments_2023-10-05_15-01-54
parser.add_argument('--experiment', type=str, required=True)
parser.add_argument('--projectdir', type=str, required=True)
args = parser.parse_args()

output_dir = os.path.join(args.projectdir, 'build/output/')
df = files_to_dataframe(output_dir, args.experiment)

# Sort from lower to higher resolution
df = df.sort_values(by='population_size', ascending=True)

# Prepare observed hopsitalization data
real_datapath = os.path.join(args.projectdir, 'src/data/observed_hospital_admissions_per_day.csv')
real_hospitalized = pd.read_csv(real_datapath)
real_hospitalized.loc[-1] = [0]  # adding a row
real_hospitalized.index = real_hospitalized.index + 1  # shifting index
real_hospitalized.sort_index(inplace=True) 
real_hospitalized.drop(real_hospitalized.tail(1).index,inplace=True) # drop last n rows
real_hospitalized = real_hospitalized.iloc[:, 0].tolist()
days = [24*i for i in range(0, len(real_hospitalized))]

fig, axs = plt.subplots(1, 3, figsize=(20, 6))
plt.subplots_adjust(wspace=0.3)
i = 0
for index, row in df.iterrows():
    x_values = list(range(len(row['ts_hospitalized'])))
    axs[i].plot(days, real_hospitalized, label='Observed Hospitalized', color=color_palette[2])
    axs[i].plot(x_values, row['ts_hospitalized'], label='Hospitalized', color=color_palette[3])
    
    if "ts_hospitalized_error_low" in df.columns and "ts_hospitalized_error_high" in df.columns:
        lower_error = [x - y for x, y in zip(row['ts_hospitalized'], row['ts_hospitalized_error_low'])]
        upper_error = [x + y for x, y in zip(row['ts_hospitalized'], row['ts_hospitalized_error_high'])]
        axs[i].fill_between(x_values, lower_error, upper_error, alpha=0.2, color=color_palette[3], label='Error Range')

    axs[i].set_xlabel('Time')
    axs[i].set_ylabel('Hospital admissions')
    axs[i].tick_params('y')
    axs[i].legend(loc='upper left')
    axs[i].set_ylim(bottom=0, top=850)
    axs[i].grid()

    ax2 = axs[i].twinx()
    ax2.plot(range(len(row['ts_exposed'])), row['ts_exposed'], label='Exposed', color=color_palette[1])
    ax2.plot(range(len(row['ts_infectious'])), row['ts_infectious'], label='Infectious', color=color_palette[0])
    ax2.get_yaxis().set_major_formatter(plt.FuncFormatter(lambda x, loc: "{:,}".format(int(x/1000))))

    ax2.set_ylabel('Exposed / Infectious (x1000)')
    ax2.tick_params('y')
    ax2.legend(loc='upper right')
    current_max = max(row['ts_infectious'])
    ax2.set_ylim(bottom=0, top=current_max * 1.1)
    
    # plt.title('Line Plot of ts_hospitalized')

    # subtitle_text = f"Simulation {row['uuid']}"
    # plt.annotate(subtitle_text, (0.5, 1.1), xycoords='axes fraction', ha='center', va='center', fontsize=12)
    i += 1

figure_dir = os.path.join(args.projectdir, 'build/output/figures')
if not os.path.isdir(figure_dir):
    os.makedirs(figure_dir)
plt.savefig(os.path.join(figure_dir, 'resolution_comparison.png'))
