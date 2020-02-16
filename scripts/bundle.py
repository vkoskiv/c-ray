#!/usr/local/bin/python3

# Copyright (c) 2020, Valtteri Koskivuori
# Created on 31.1.2020

# This script will receive a given .json scene description, and generate
# a .zip bundle of that description and all the assets it references.
# This way you can share scenes you've created for c-ray in a more portable way.

import sys
import os
import shutil
import json
import datetime

datestring = datetime.datetime.now().replace(microsecond=0).isoformat()

def mtls_from_obj(objfilename):
	mtlfiles = []
	if not os.path.isfile(objfilename):
		return
	with open(objfilename, 'r') as f:
		data = f.readlines()
		for line in data:
			if line.startswith('mtllib'):
				mtlfiles.append(line.split(' ')[1].strip('\n'))
		f.close()
	return mtlfiles

def textures_from_mtl(mtlfilename):
	textures = []
	with open(mtlfilename, 'r') as f:
		data = f.readlines()
		for line in data:
			if line.startswith('map_Kd'):
				textures.append(line.split(' ')[1].strip('\n'))
			elif line.startswith('norm'):
				textures.append(line.split(' ')[1].strip('\n'))
	return textures

def just_path(fullpath):
	return os.path.join(os.path.dirname(fullpath), '')

def mkdir_if_need(path):
	if not os.path.exists(path):
		print('mkdir: ' + path)
		os.makedirs(path)

def copy_file(src, dest):
	print('Copying ' + src + ' to ' + dest)
	shutil.copy2(src, dest)

if __name__ == '__main__':
	# Do the thing
	print("C-ray scene bundler v0.1")
	if len(sys.argv) != 2:
		print("Usage: ", str(sys.argv[0]), "<somefile>.json")
		exit()
	print("Packing file", str(sys.argv[1]))
	filename = sys.argv[1]
	assetPath = just_path(sys.argv[1])
	destPath = os.path.splitext(os.path.basename(filename))[0] + '-' + datestring

	with open(filename) as f:
		jsondata = json.load(f)
	mkdir_if_need(destPath)

	# Copy the main JSON itself
	copy_file(sys.argv[1], destPath)

	# Copy HDR (if applicable)
	if 'scene' in jsondata:
		if 'ambientColor' in jsondata['scene']:
			if 'hdr' in jsondata['scene']['ambientColor']:
				dest_offset = just_path(jsondata['scene']['ambientColor']['hdr'])
				mkdir_if_need(destPath + '/' + dest_offset)
				copy_file(assetPath + jsondata['scene']['ambientColor']['hdr'], destPath + '/' + dest_offset)

	# Copy meshes and related data
	if 'scene' in jsondata:
		if 'meshes' in jsondata['scene']:
			for mesh in jsondata['scene']['meshes']:
				if 'fileName' in mesh:
					if os.path.isfile(assetPath + mesh['fileName']):
						mtls = mtls_from_obj(assetPath + mesh['fileName'])
						dest_offset = just_path(mesh['fileName'])
						mkdir_if_need(destPath + '/' + dest_offset)
						copy_file(assetPath + mesh['fileName'], destPath + '/' + dest_offset)
						path_offset = just_path(mesh['fileName'])
						if mtls is not None:
							for mtl in mtls:
								dest_offset = just_path(mtl)
								mkdir_if_need(destPath + '/' + path_offset + dest_offset)
								copy_file(assetPath + path_offset + mtl, destPath + '/' + path_offset + dest_offset)
								texs = textures_from_mtl(assetPath + path_offset + mtl)
								for tex in texs:
									mkdir_if_need(destPath + '/' + path_offset + dest_offset)
									copy_file(assetPath + tex, destPath + '/' + path_offset + dest_offset)
	shutil.make_archive(destPath, 'zip', destPath)
	shutil.rmtree(destPath)
	print('\n\nBundle ' + destPath + '.zip created!')
	example = os.path.splitext(os.path.basename(filename))[0]
	print('To use it, do \'unzip -d ' + example + ' ' + destPath +'.zip\'')
	print('And then \'./bin/c-ray ' + example +'/' + example + '.json\'')
