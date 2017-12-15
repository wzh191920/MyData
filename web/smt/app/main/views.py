import os
from flask import render_template, redirect, abort, url_for, flash, request, current_app
from . import main
from flask.helpers import make_response
import Mydata
from Mydata import lib, PI, handle, c_int
from ..decorators import log_error


@main.route('/', methods=['GET', 'POST'])
def index():
    table_num = PI(c_int(0))
    err = lib.MyDataGetTableNum(handle, table_num)
    if err:
        current_app.logger.error("获取表数量失败,%d", err)
    table_info = (Mydata.TableInfo*table_num.contents.value)()
    err = lib.MyDataGetTables(handle, table_info, table_num)
    if err:
        current_app.logger.error("获取表信息失败,%d", err)
    table_num = table_num.contents.value
    return render_template('index.html', table_num=table_num, table_info=table_info)

@main.route('/show_types')
def show_types():
    num = PI(c_int(0))
    err = lib.MyDataGetTypeNum(handle, num)
    if err:
        current_app.logger.error("获取类型数量失败,%d", err)
    typeinfo = (Mydata.TypeInfo*num.contents.value)()
    err = lib.MyDataGetTypes(handle, typeinfo, num)
    if err:
        current_app.logger.error("获取类型信息失败,%d", err)
    typestrs = Mydata.Typeid2Typestr(typeinfo)
    return render_template('types.html', typeinfo = typeinfo, titlename = '类型信息', num=num.contents.value, typestrs = typestrs)

@main.route('/add_type')
def add_type():
    return render_template('add_type.html', titlename = '创建类型')

@main.route('/show_datas/<int:pointid>/<int:typeid>')
def show_datas(pointid, typeid):
    return render_template('datas.html', id=pointid, typeid=typeid, titlename = '数据查询')

@main.route('/show_datafiles')
def show_datafiles():
    num = PI(c_int())
    err = lib.GetDataFileNums(handle, num)
    if err:
        current_app.logger.error("获取数据文件数量失败,%d", err)
    dfinfo = (Mydata.DataFileInfo*num.contents.value)()
    err = lib.GetDataFileInfos(handle, dfinfo, num)
    
    if err:
        current_app.logger.error("获取数据文件信息失败,%d", err)
    return render_template('datafiles.html', dfinfo = dfinfo, titlename = '数据文件信息', num = num.contents.value)