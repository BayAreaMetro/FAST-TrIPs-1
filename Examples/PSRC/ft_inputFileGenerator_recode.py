###################################################################################
#   Transit Input File Generator
#   Created on September 9, 2013
#   By Alireza Khani (akhani@email.arizona.edu)
#   Reads GTFS Files
#   Writes ft_input_stops.dat, ft_input_trips.dat, ft_input_routes.dat, ft_input_stopTimes.dat, ft_input_shapes.dat
###################################################################################
import datetime
import sys
import pandas as pd
import numpy as np
import json
###################################################################################

year=int(raw_input("Please enter year (yyyy): "))
month=int(raw_input("Please enter month (mm): "))
day=int(raw_input("Please enter day (dd): "))
date = year*10000 + month*100 + day
dayOfWeek = datetime.date(year, month, day).isoweekday()
###############################################################################


def make_sequence_col(data_frame, sort_list, group_by_col, seq_col):

    """Sorts a pandas dataframe using sort_list, then creates a column of sequential integers (1,2,3, etc.) for
        groups, using the group_by_col. Then drops the existing sequence column and re-names the new sequence column.

    """
    
    #sort on tripId, sequence
    data_frame = data_frame.sort(sort_list, ascending=[1,1])
    
    #create a new field, set = to the position of each record in a group, grouped by tripId
    data_frame['temp'] = data_frame.groupby(group_by_col).cumcount() + 1
    #drop the old sequence column
    data_frame = data_frame.drop(seq_col, axis=1)
    #rename new column:
    data_frame = data_frame.rename(columns = {'temp':seq_col})

    return data_frame

def rename_fields(data_frame, table_name):

    """Renames the fields in a pandas data_frame using the 'table_name' and 'recode' keys in the configuarion dictionary (global variable).
    Also checks to see if there are missing columns and columns that have no data. Populates these cases with '_'.

    """
    for new_name, old_name in config[table_name]['recode'].iteritems():
        if old_name not in data_frame.columns:
            data_frame[old_name] = '_'
        if data_frame[old_name].isnull().sum() == len(data_frame[old_name]):
            print 'Warning: ' + old_name + ' in ' + table_name + ' contains cells with no data. All values will be converted to _' 
            data_frame[old_name] = '_'
        data_frame = data_frame.rename(columns = {old_name : new_name})
    return data_frame



with open('config.json', 'r') as f:
    config = json.load(f)

df_calendar = pd.read_csv('calendar.txt')
df_calendar = df_calendar.query(str(date) + ' >= start_date and ' + str(date) + ' <= end_date')
#if date >= df_calendar['start_date'].min() and date <= df_calendar['end_date'].max():
if df_calendar.empty:
    print 'Invalid Dates'
    exit()
else: 
    dayOfWeek = df_calendar.columns[dayOfWeek]
    df_selected_service = df_calendar.query(dayOfWeek + ' == 1')
    service = list(df_selected_service['service_id'])
    print service

df_trips = pd.read_csv('trips.txt')
df_trips = df_trips[df_trips['service_id'].isin(service)]


#list of unique trip_ids, route_id, shape_id
tripIds = np.unique(df_trips['trip_id'])
routeIds = np.unique(df_trips['route_id'])
shapeIds = np.unique(df_trips['shape_id'])

#routes
df_routes = pd.read_csv('routes.txt')
df_routes = df_routes[df_routes['route_id'].isin(routeIds)]
df_routes['route_long_name'].fillna('_', inplace=True)

#stop_times
df_stop_times = pd.read_csv('stop_times.txt')
df_stop_times = df_stop_times[df_stop_times['trip_id'].isin(tripIds)]

#remove ':' from time fields:
df_stop_times['arrival_time'].replace(regex=True,inplace=True,to_replace=r'\D',value=r'')
df_stop_times['departure_time'].replace(regex=True,inplace=True,to_replace=r'\D',value=r'')

#sometimes stop_sequence does not start with 1 and increment by 1
df_stop_times = make_sequence_col(df_stop_times, ['trip_id', 'stop_sequence'], 'trip_id', 'stop_sequence')
stopIds = np.unique(df_stop_times['stop_id'])
#df_stop_times['arrival_time'] = df_stop_times['arrival_time'].map(lambda x: x.lstrip(':').rstrip('aAbBcC'))

#stops
df_stops = pd.read_csv('stops.txt')
df_stops = df_stops[df_stops['stop_id'].isin(stopIds)]

#shapes
df_shapes = pd.read_csv('shapes.txt')
df_shapes = df_shapes[df_shapes['shape_id'].isin(shapeIds)]


print "%i trips, %i routes, %i stop times, %i stops, %i shapes!" %(len(df_trips), len(df_routes), len(df_stop_times), len(df_stops), len(df_shapes))
#print len(service), len(trips), len(routes), len(stopTimes), len(stops), len(shapeIds), len(shapes)
################################################################################
FT=raw_input("Do you want to prepare input files for FAST-TrIPs? (y/n): ")
while FT not in ["y","n"]:
    FT=raw_input("Do you want to prepare input files for FAST-TrIPs? (y/n): ")

if FT=="n":
    sys.exit()

######## Write out stops file #######
df_stops = rename_fields(df_stops, 'stops')

#add a capacity field, set to 100?
df_stops['capacity'] = 100

df_stops.to_csv('ft_input_stops.dat', 
                 columns = config['stops']['field_seq'], 
                 index = False, sep='\t') 

######## Write out routes file #######
df_routes = rename_fields(df_routes, 'routes')
df_routes.to_csv('ft_input_routes.dat', 
                 columns = config['routes']['field_seq'], index = False, sep='\t') 


######## Write out trips file #######
#new data frame with trip_id and the first stop departure time. Check to make sure correct. 

df_min_stop_time = df_stop_times.sort(['trip_id', 'stop_sequence'],  ascending=[1,1])

df_min_stop_time = df_min_stop_time.sort('stop_sequence', ascending=True).groupby('trip_id', as_index=False).first()

#need min stop time
df_trips = pd.merge(left = df_trips, right = df_min_stop_time, on=['trip_id'])

#need route_type from routes
df_routes = df_routes.rename(columns = {'routeId' : 'route_id'})
df_trips = pd.merge(left = df_trips, right = df_routes, on=['route_id'])
df_trips = rename_fields(df_trips, 'trips')
df_trips['capacity'] = 60
df_trips.to_csv('ft_input_trips.dat', 
                 columns = config['trips']['field_seq'], index = False, sep='\t') 

######## Write out stops times file #######
df_stop_times = rename_fields(df_stop_times, 'stop_times')
df_stop_times.to_csv('ft_input_stopTimes.dat', 
                 columns = config['stop_times']['field_seq'], index = False, sep='\t') 

######## Write out shapes file #######
df_shapes = rename_fields(df_shapes, 'shapes')
df_shapes.to_csv('ft_input_shapes.dat', 
                 columns = config['shapes']['field_seq'], index = False, sep='\t') 

print "Done!"

