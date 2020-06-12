﻿#include "Utilities.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <QSettings>
#include <QPainter>
#include <QIcon>
#include <QOpenGLFunctions_4_5_Core>

#include "Texture.h"
#include "ResourceManager.h"

QOpenGLFunctions_4_5_Core* gl;
Shapes shapes;

void Shapes::init() {
	gl->glCreateBuffers(1, &vertex_buffer);
	gl->glNamedBufferData(vertex_buffer, quad_vertices.size() * sizeof(glm::vec2), quad_vertices.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, quad_indices.size() * sizeof(unsigned int) * 3, quad_indices.data(), GL_STATIC_DRAW);
}

std::string string_replaced(const std::string& source, const std::string& from, const std::string& to) {
	std::string new_string;
	new_string.reserve(source.length());  // avoids a few memory allocations

	size_t lastPos = 0;
	size_t findPos;

	while (std::string::npos != (findPos = source.find(from, lastPos))) {
		new_string.append(source, lastPos, findPos - lastPos);
		new_string += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	new_string += source.substr(lastPos);

	return new_string;
}

std::vector<std::string> split(const std::string& string, const char delimiter) {
	std::vector<std::string> elems;
	std::stringstream ss(string);

	std::string item;
	while (std::getline(ss, item, delimiter)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string_view> splitSV(std::string_view str, const char delimiter) {
	std::vector<std::string_view> output;
	output.reserve(str.size() / 2);

	for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last; first = second + 1) {
		while (++second < last && *second != delimiter) {}
		if (first != second) {
			output.emplace_back(first, second - first);
		}
	}

	return output;
}

std::string to_lowercase_copy(const std::string_view& string) {
	std::string output(string);
	std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });
	return output;
}

void to_lowercase(std::string& string) {
	std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) { return std::tolower(c); });
}


// trim from start (in place)
void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

bool is_number(const std::string& s) {
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader) {
	char buffer[512];
	GLint status;

	std::string vertex_source = read_text_file(vertex_shader.string());
	std::string fragment_source = read_text_file(fragment_shader.string());

	const GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	const char* source = vertex_source.c_str();
	gl->glShaderSource(vertex, 1, &source, nullptr);
	gl->glCompileShader(vertex);


	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(vertex, 512, nullptr, buffer);
	if (!status) {
		std::cout << buffer << std::endl;
	}

	// Fragment Shader
	source = fragment_source.c_str();
	gl->glShaderSource(fragment, 1, &source, nullptr);
	gl->glCompileShader(fragment);

	gl->glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(fragment, 512, nullptr, buffer);
	if (!status) {
		std::cout << buffer << std::endl;
	}

	// Link
	const GLuint shader = gl->glCreateProgram();
	gl->glAttachShader(shader, vertex);
	gl->glAttachShader(shader, fragment);
	gl->glLinkProgram(shader);

	gl->glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(shader, 512, nullptr, buffer);
		std::cout << buffer << std::endl;
	}

	gl->glDeleteShader(vertex);
	gl->glDeleteShader(fragment);

	return shader;
}

std::string read_text_file(const fs::path& path) {
	std::ifstream textfile(path.c_str());
	std::string line;
	std::string text;

	if (!textfile.is_open())
		return "";

	while (getline(textfile, line)) {
		text += line + "\n";
	}

	return text;
}

fs::path find_warcraft_directory() {
	QSettings settings;
	if (settings.contains("warcraftDirectory")) {
		return settings.value("warcraftDirectory").toString().toStdString();
	} else if (fs::exists("C:/Program Files (x86)/Warcraft III")) {
		return "C:/Program Files (x86)/Warcraft III";
	} else if (fs::exists("D:/Program Files (x86)/Warcraft III")) {
		return "D:/Program Files (x86)/Warcraft III";
	} else {
		return "";
	}
}

void load_modification_file(const std::string file_name, slk::SLK2& base_data, slk::SLK2& meta_slk, bool optional_ints) {
	BinaryReader reader = hierarchy.map_file_read(file_name);

	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2) {
		std::cout << "Unknown modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, base_data, meta_slk, false, optional_ints);
	load_modification_table(reader, base_data, meta_slk, true, optional_ints);
}

void load_modification_table(BinaryReader& reader, slk::SLK2& slk, slk::SLK2& meta_slk, const bool modification, bool optional_ints) {
	const uint32_t objects = reader.read<uint32_t>();
	for (size_t i = 0; i < objects; i++) {
		const std::string original_id = reader.read_string(4);
		const std::string modified_id = reader.read_string(4);

		if (modification) {
			slk.copy_row(original_id, modified_id, false);
			// Required when saving the modification table again
			slk.set_shadow_data("oldid", modified_id, original_id);
		}

		const uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			const std::string modification_id = reader.read_string(4);
			const uint32_t type = reader.read<uint32_t>();

			std::string column_header = to_lowercase_copy(meta_slk.data("field", modification_id));
			if (optional_ints) {
				uint32_t level_variation = reader.read<uint32_t>();
				uint32_t data_pointer = reader.read<uint32_t>();
				if (level_variation != 0) {
					if (meta_slk.column_headers.contains("data") && meta_slk.data<int>("data", modification_id) > 0) {
						column_header += char('a' + data_pointer);
					}
					column_header += std::to_string(level_variation);
				}
			}

			std::string data;
			switch (type) {
				case 0:
					data = std::to_string(reader.read<int>());
					break;
				case 1:
				case 2:
					data = std::to_string(reader.read<float>());
					break;
				case 3:
					data = reader.read_c_string();
					break;
				default: 
					std::cout << "Unknown data type " << type << " while loading modification table.";
			}
			reader.advance(4);

			if (column_header == "") {
				std::cout << "Unknown mod id: " << modification_id << "\n";
				continue;
			}

			if (modification) {
				slk.set_shadow_data(column_header, modified_id, data);
			} else {
				slk.set_shadow_data(column_header, original_id, data);
			}
		}
	}
}

void save_modification_file(const std::string file_name, slk::SLK2& slk, slk::SLK2& meta_slk, bool optional_ints) {
	BinaryWriter writer;
	writer.write<uint32_t>(mod_table_write_version);

	save_modification_table(writer, slk, meta_slk, false, optional_ints);
	save_modification_table(writer, slk, meta_slk, true, optional_ints);

	hierarchy.map_file_write(file_name, writer.buffer);
}

// The idea of SLKs and mod files is quite bad, but I can deal with them
// The way they are implemented is horrible though
void save_modification_table(BinaryWriter& writer, slk::SLK2& slk, slk::SLK2& meta_slk, bool custom, bool optional_ints) {
	// Create an temporary index to speed up field lookups
	absl::flat_hash_map<std::string, std::string> meta_index;
	for (const auto& [key, dontcare2] : meta_slk.row_headers) {
		if (meta_index.contains(to_lowercase_copy(meta_slk.data("field", key)))) {
			puts("s");
		}
		meta_index[to_lowercase_copy(meta_slk.data("field", key))] = key;
	}

	BinaryWriter sub_writer;

	size_t count = 0;
	for (const auto& [id, properties] : slk.shadow_data) {
		// If we are writing custom objects then we only want rows with oldid set as the others are base rows
		if (!custom && properties.contains("oldid")) {
			continue;
		} else if (custom && !properties.contains("oldid")) {
			continue;
		}
		count++;

		if (custom) {
			sub_writer.write_string(properties.at("oldid"));
			sub_writer.write_string(id);
		} else {
			sub_writer.write_string(id);
			sub_writer.write<uint32_t>(0);
		}

		sub_writer.write<uint32_t>(properties.size() - (properties.contains("oldid") ? 1 : 0));

		for (const auto& [property_id, value] : properties) {
			if (property_id == "oldid") {
				continue;
			}

			//// Find the metadata ID for this field name since modification files are stupid
			std::string meta_data_key;

			int variation = 0;
			if (meta_index.contains(property_id)) {
				meta_data_key = meta_index.at(property_id);
			} else {
				size_t nr_position = property_id.find_first_of("0123456789");
				std::string without_numbers = property_id.substr(0, nr_position);

				if (nr_position < property_id.size()) {
					variation = std::stoi(property_id.substr(nr_position));
				}

				if (meta_index.contains(without_numbers)) {
					meta_data_key = meta_index.at(without_numbers);
				}
			}

			if (meta_data_key.empty()) {
				puts("s");
			}

			sub_writer.write_string(meta_data_key);
			
			int write_type = -1;
			const std::string type = meta_slk.data("type", meta_data_key);
			if (type == "int" 
				|| type == "bool" 
				|| type.ends_with("Flags") 
				|| type == "attackBits" 
				|| type == "channelType" 
				|| type == "deathType" 
				|| type == "defenseTypeInt" 
				|| type == "detectionType" 
				|| type == "spellDetail" 
				|| type == "teamColor" 
				|| type == "techAvail") {

				write_type = 0;
			} else if (type == "real") {
				write_type = 1;
			} else if (type == "unreal") {
				write_type = 2;
			} else { // string
				write_type = 3;
			}

			sub_writer.write<uint32_t>(write_type);

			if (optional_ints) {
				sub_writer.write<uint32_t>(variation);
				sub_writer.write<uint32_t>(0); // 🤔
			}

			if (write_type == 0) {
				sub_writer.write<int>(std::stoi(value));
			} else if (write_type == 1 || write_type == 2) {
				sub_writer.write<float>(std::stof(value));
			} else {
				sub_writer.write_c_string(value);
			}

			sub_writer.write<uint32_t>(0);
		}
	}

	writer.write<uint32_t>(count);
	writer.write_vector(sub_writer.buffer);
}

QIcon ground_texture_to_icon(uint8_t* data, const int width, const int height) {
	QImage temp_image = QImage(data, width, height, QImage::Format::Format_RGBA8888);
	const int size = height / 4;

	auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

	QIcon icon;
	icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

	QPainter painter(&pix);
	painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
	painter.end();

	icon.addPixmap(pix, QIcon::Normal, QIcon::On);

	return icon;
}

QIcon texture_to_icon(fs::path path) { 
	auto tex = resource_manager.load<Texture>(path);
	QImage temp_image = QImage(tex->data.data(), tex->width, tex->height, tex->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
	auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
};