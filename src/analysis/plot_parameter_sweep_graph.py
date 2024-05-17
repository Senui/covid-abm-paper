import plotly.express as px
import plotly.graph_objs as go
import psycopg2
import pandas as pd
import dash, os
from dash import dcc
from dash import html
from dash.dependencies import Input, Output
import argparse

from experiments_to_dataframe import files_to_dataframe

slider_params = ['beta1', 'population_size', 'custom_agent_to_person_ratio', 'hospitalization_scale_param', 'hospital_average_mean']
# slider_params = ['beta1', 'population_size', 'init_infection_rate', 'initial_exposed_infected_ratio', 'init_infection_time']

# Create arg parser
parser = argparse.ArgumentParser()
parser.add_argument('--projectdir', type=str, required=True)
# e.g. --experiment=experiments_2023-10-05_15-01-54
parser.add_argument('--experiment', type=str)
args = parser.parse_args()


def find_last_created_experiment(base_directory):
    subdirectories = [dir for dir in os.listdir(base_directory) if os.path.isdir(os.path.join(base_directory, dir)) and dir.startswith("experiments_")]
    subdirectories.sort(key=lambda dir: os.path.getctime(os.path.join(base_directory, dir)), reverse=True)
    for subdir in subdirectories:
        if os.listdir(os.path.join(base_directory, subdir)):
            return subdir
    return None

output_dir = os.path.join(args.projectdir, 'build/output/')
print(f"Importing experiments from {output_dir}")
# if no specific experiment is requested we select the latest (non-empty) one
if not args.experiment:
    experiment = find_last_created_experiment(output_dir)
    print(f"Using last found experiment: {experiment}")
else:
    experiment = args.experiment
df = files_to_dataframe(output_dir, experiment)

print(df)

real_datapath = os.path.join(args.projectdir, 'src/data/observed_hospital_admissions_per_day.csv')
real_hospitalized = pd.read_csv(real_datapath)
real_hospitalized.loc[-1] = [0]  # adding a row
real_hospitalized.index = real_hospitalized.index + 1  # shifting index
real_hospitalized.sort_index(inplace=True) 
real_hospitalized.drop(real_hospitalized.tail(1).index,inplace=True) # drop last n rows
real_hospitalized = real_hospitalized.iloc[:, 0].tolist()
days = [24*i for i in range(0, len(real_hospitalized))]

# Initialize the dash app
app = dash.Dash()

def get_slider_name(param):
    return param + '-slider'

def create_slider_from_param(param):
    slider_name = get_slider_name(param)
    slider = dcc.Slider(id=slider_name, min=df[param].min(), max=df[param].max(), updatemode='drag', step=None, value=df[param].min(), marks={str(b1):str(b1) for b1 in df[param].unique()})
    ret = html.Div([html.Label(param), slider])
    return ret

# Define the background spans
background_spans = [
    dict(x0=0, x1=336, y0=0, y1=1, fillcolor='green', opacity=0.05, yref='paper', layer='below'),
    dict(x0=337, x1=336+264, y0=0, y1=1, fillcolor='yellow', opacity=0.05, yref='paper', layer='below'),
    dict(x0=336+264+1, x1=336+264+1176, y0=0, y1=1, fillcolor='red', opacity=0.05, yref='paper', layer='below'),
    dict(x0=336+264+1176+1, x1=336+264+1176+504, y0=0, y1=1, fillcolor='blue', opacity=0.02, yref='paper', layer='below')
]


# Define the layout
div_children = [html.H1("COVID-19 Hospitalization Netherlands Feb 2020 - June 2020")]
sliders_div = []
for p in slider_params:
    sliders_div.append(create_slider_from_param(p))
div_children.append(html.Div(sliders_div, style={'display': 'inline-block', 'width': '40%', 'vertical-align': 'top', 'margin-top': '80px'}))
div_children.append(html.Div([dcc.Graph(id='ts_hospitalized')], style={'display': 'inline-block', 'width': '60%', 'vertical-align': 'top'}))
div_children.append(html.Div([dcc.Graph(id='ts_affected')], style={'display': 'inline-block', 'width': '60%', 'vertical-align': 'top'}))
app.layout = html.Div(div_children)

# Define the callbacks
input_list = [Input(component_id=get_slider_name(component), component_property='value') for component in slider_params]
@app.callback(
    Output(component_id='ts_hospitalized', component_property='figure'),
    input_list
)
def update_figure(*input_values):
    filter_conditions = [df[column_name] == value for column_name, value in zip(slider_params, input_values)]
    filtered_df = df[pd.concat(filter_conditions, axis=1).all(axis=1)]
    print(filtered_df)

    traces = []
    for index, row in filtered_df.iterrows():
        traces.append(go.Scatter(x=list(range(len(row['ts_hospitalized']))), y=row['ts_hospitalized'], name='Hospitalized', yaxis='y1'))
        if "ts_hospitalized_error_low" in df.columns and "ts_hospitalized_error_high" in df.columns:
            x_values = list(range(len(row['ts_hospitalized'])))
            lower_error = [x - y for x, y in zip(row['ts_hospitalized'], row['ts_hospitalized_error_low'])]
            upper_error = [x + y for x, y in zip(row['ts_hospitalized'], row['ts_hospitalized_error_high'])]
            traces.append(go.Scatter(x=x_values + x_values[::-1],
                            y=upper_error + lower_error[::-1],
                            fill='tozerox',
                            fillcolor='rgba(0,100,80,0.2)',
                            line=dict(color='rgba(255,255,255,0)'),
                            name='Error Range'))

    observed = go.Scatter(x=days, y=real_hospitalized, name='Observed Hospitalized', yaxis='y1')
    traces.append(observed)

    exposed = go.Scatter(x=list(range(len(row['ts_exposed']))), y=row['ts_exposed'], name='Exposed', yaxis='y2')
    traces.append(exposed)
    infected = go.Scatter(x=list(range(len(row['ts_infectious']))), y=row['ts_infectious'], name='Infectious', yaxis='y2')
    traces.append(infected)

    subtitle_text = f"Simulation {row['uuid']}"

    return {
        'data': traces,
        'layout': go.Layout(title=f'Line Plot of ts_hospitalized', shapes=background_spans, xaxis_title='Time', yaxis=dict(title='Hospital admissions', range=[0,800]),
                       yaxis2=dict(title='Exposed',
                                   overlaying='y',
                                   side='right'),
        annotations=[{
                    'x': 0.5,
                    'y': 1.1,
                    'xref': 'paper',
                    'yref': 'paper',
                    'text': subtitle_text,
                    'showarrow': False,
                    'font': {'size': 12}
                }])
    }

# Callback on affected graph
input_list = [Input(component_id=get_slider_name(component), component_property='value') for component in slider_params]
@app.callback(
    Output(component_id='ts_affected', component_property='figure'),
    input_list
)
def update_figure(*input_values):
    filter_conditions = [df[column_name] == value for column_name, value in zip(slider_params, input_values)]
    filtered_df = df[pd.concat(filter_conditions, axis=1).all(axis=1)]

    traces = []
    for index, row in filtered_df.iterrows():
        for column_name, column_value in row.items():
            if column_name.startswith('ts_affected'):
                traces.append(go.Scatter(x=list(range(len(column_value))), y=column_value, name=column_name, yaxis='y1'))

    return {
        'data': traces,
        'layout': go.Layout(title=f'Line Plot of ts_affected_*', xaxis_title='Time', yaxis_title='Percentage affected')
    }

# Run the app
if __name__ == '__main__':
    app.run_server(debug=True)
