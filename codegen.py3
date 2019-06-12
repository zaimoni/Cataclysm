#!/usr/bin/python
# designed for Python 3.7.0, may work with other versions
# (C)2018-2019 Kenneth Boyd, license: MIT.txt

from os import replace

def LHS_of_assignment(x):
	n = x.find('=')
	if -1<n:
		return x[0:n].strip()
	return x

def bracketed_lines(srcfile,begin_flag,end_flag):
	have_seen = False
	relevant_lines = []				# return this?
	src = open(srcfile,'r')		# likely parameter
	# read in src; obtain all enum identifiers and prepare functions based on that
	for line in src:
		# remove C++ one-line comments and leading/trailing whitespace
		n = line.find('//')
		clean_line = line.strip() if -1 ==n  else line[0:n].strip()
		# identify whether line is 'interesting'
		if ''==clean_line:
			continue
		if have_seen:
			if clean_line.startswith(end_flag):	# likely parameter
				break
			relevant_lines.append(clean_line)
			continue
		if clean_line.startswith(begin_flag):	# likely parameter
			have_seen = True
			clean_line = clean_line[len(begin_flag):].strip()
			if ''==clean_line:
				continue
			relevant_lines.append(clean_line)

	src.close()
	return relevant_lines

def extract_C_enum(src):
	enum_like = []
	for x in src:
		n = x.find(',')
		if -1==n:
			enum_like.append(LHS_of_assignment(x))
			continue;
		candidates = x.split(',')
		for y in candidates:
			clean_y = LHS_of_assignment(y.strip())
			if ''!=clean_y:
				enum_like.append(clean_y)
	return enum_like

def JSON_transcode_definition(src):
	JSON_diagnosis_array_body = []
	for x in src[:-1]:
		JSON_diagnosis_array_body.append('"'+x+'"')
	return 'static const char* JSON_transcode['+src[-1]+'] = {\n\t'+',\n\t'.join(JSON_diagnosis_array_body)+'\n};'

def JSON_transcode_definition_pair(src):
	JSON_diagnosis_array_body = []
	for x in src:
		JSON_diagnosis_array_body.append('{'+x+',"'+x+'"}')
	return 'static const std::pair<foo,const char*> JSON_transcode[] = {\n\t'+',\n\t'.join(JSON_diagnosis_array_body)+'\n};'

#relevant_lines = bracketed_lines('bionics.h','enum bionic_id {','}')
# relevant_lines = bracketed_lines('color.h','enum nc_color {','}')
# relevant_lines = bracketed_lines('computer.h','enum computer_action','}')
# relevant_lines = bracketed_lines('computer.h','enum computer_failure','}')
# relevant_lines = bracketed_lines('itype.h','enum itype_id {','}')
# relevant_lines = bracketed_lines('mapdata.h','enum ter_id {','}')
relevant_lines = bracketed_lines('mission.h','enum mission_id {','}')
# relevant_lines = bracketed_lines('mtype.h','enum mon_id {','}')
# relevant_lines = bracketed_lines('npc.h','enum npc_favor_type {','}')
# relevant_lines = bracketed_lines('skill.h','enum skill {','}')
enum_like = extract_C_enum(relevant_lines)
print(JSON_transcode_definition(enum_like))
# print(JSON_transcode_definition_pair(enum_like))

