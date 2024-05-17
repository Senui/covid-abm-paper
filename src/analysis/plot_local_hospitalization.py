import argparse, os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

from experiments_to_dataframe import files_to_dataframe

def mean_list_of_lists(data):
    data_t = np.array(data).T
    mean = np.mean(data_t, axis=1)
    return mean

color_palette = ['#e66101','#fdb863','#b2abd2','#5e3c99']

# Create arg parser
parser = argparse.ArgumentParser()
# e.g. --experiment=experiments_2023-10-05_15-01-54
parser.add_argument('--experiment', type=str, required=True)
parser.add_argument('--projectdir', type=str, required=True)
args = parser.parse_args()

output_dir = os.path.join(args.projectdir, 'build/output/')
df = files_to_dataframe(output_dir, args.experiment)

# Column names of data of interest
local_municipalities = ['eindhoven', 'groningen', 'denhaag']

fig, axs = plt.subplots(1, 3, figsize=(20, 6))
plt.subplots_adjust(wspace=0.3)
i = 0
for m in local_municipalities:
    y_values_all = df[f'ts_hospitalized_{m}'].tolist()
    y_values = mean_list_of_lists(y_values_all)
    x_values = list(range(len(y_values)))

    real_data_path = os.path.join(args.projectdir, f'src/data/observed_hospital_admissions_per_day_{m}.csv')
    real_municipality_data = pd.read_csv(real_data_path, header=None)
    real_municipality_data = real_municipality_data[0].to_numpy()
    days = [24*i for i in range(0, len(real_municipality_data))]

    axs[i].plot(days, real_municipality_data, label='Observed Hospitalized', color=color_palette[2])
    axs[i].plot(x_values, y_values, label='Hospitalized', color=color_palette[3])
    
    if f"ts_hospitalized_{m}_error_low" in df.columns and f"ts_hospitalized_{m}_error_high" in df.columns:
        std_low_all = df[f'ts_hospitalized_{m}_error_low'].to_list()
        std_high_all = df[f'ts_hospitalized_{m}_error_high'].to_list()
        std_low = mean_list_of_lists(std_low_all)
        std_high = mean_list_of_lists(std_high_all)
        lower_error = [x - y for x, y in zip(y_values, std_low)]
        upper_error = [x + y for x, y in zip(y_values, std_high)]
        axs[i].fill_between(x_values, lower_error, upper_error, alpha=0.2, color=color_palette[3], label='Error Range')
    else:
        std_values =  np.std(y_values_all, axis=0)
        print(std_values)
        lower_error = [x - y for x, y in zip(y_values, std_values)]
        upper_error = [x + y for x, y in zip(y_values, std_values)]
        axs[i].fill_between(x_values, lower_error, upper_error, alpha=0.2, color=color_palette[3], label='Error Range')

    axs[i].set_xlabel('Time')
    axs[i].set_ylabel('Hospital admissions')
    axs[i].tick_params('y')
    axs[i].legend(loc='upper left')
    axs[i].set_ylim(bottom=0, top=15)
    axs[i].grid()
    title = m[0].upper() + m[1:]
    axs[i].set_title(f"Hospital admissions in {title}")

    # ax2 = axs[i].twinx()
    # ax2.plot(range(len(row['ts_exposed'])), row['ts_exposed'], label='Exposed', color=color_palette[1])
    # ax2.plot(range(len(row['ts_infectious'])), row['ts_infectious'], label='Infectious', color=color_palette[0])
    # ax2.get_yaxis().set_major_formatter(plt.FuncFormatter(lambda x, loc: "{:,}".format(int(x/1000))))

    # ax2.set_ylabel('Exposed / Infectious (x1000)')
    # ax2.tick_params('y')
    # ax2.legend(loc='upper right')
    # current_max = max(row['ts_infectious'])
    # ax2.set_ylim(bottom=0, top=current_max * 1.1)
    
    # plt.title('Line Plot of ts_hospitalized')

    # subtitle_text = f"Simulation {row['uuid']}"
    # plt.annotate(subtitle_text, (0.5, 1.1), xycoords='axes fraction', ha='center', va='center', fontsize=12)
    i += 1

figure_dir = os.path.join(args.projectdir, 'build/output/figures')
if not os.path.isdir(figure_dir):
    os.makedirs(figure_dir)
plt.savefig(os.path.join(figure_dir, f'local_hospitalization_{args.experiment}.png'))
