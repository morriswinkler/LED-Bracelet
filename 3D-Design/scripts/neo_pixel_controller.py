import bpy
from bpy.app.handlers import persistent
from random import random


#############################
# globals

_objects = bpy.data.objects
_spectrum_part = 0
_color_idx = 0
cur_color_granularity = 0

def load_handler(dummy):
    cur_color_granularity = random()
    cur_color_granularity = cur_color_granularity / 10
    color = getColor(cur_color_granularity)
    for ob in _objects:
        if ob.type == 'MESH' and ob.name.split('.')[0] == 'LED':
            ob.color = color
            

def getColor(cur_color_granularity):
    global _spectrum_part
    global _color_idx
    if _spectrum_part == 0:
        color = [1 - _color_idx, _color_idx, 0.0, 1.0]
        _color_idx += cur_color_granularity
        if _color_idx > 1:
            _spectrum_part = 1
            _color_idx = 0
    elif _spectrum_part == 1:
        color = [0.0, 1 - _color_idx, _color_idx, 1.0]            
        _color_idx += cur_color_granularity
        if _color_idx > 1:
            _spectrum_part = 2
            _color_idx = 0
    elif _spectrum_part == 2:
        color = [_color_idx, 0.0, 1 - _color_idx, 1.0]            
        _color_idx += cur_color_granularity
        if _color_idx > 1:
            _spectrum_part = 0
            _color_idx = 0
           
    return color
                    
bpy.app.handlers.frame_change_pre.append(load_handler)