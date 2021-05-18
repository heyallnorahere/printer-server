#include <restbed>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
static std::string read_file(std::ifstream& stream) {
    std::stringstream content;
    std::string line;
    stream.seekg(0);
    while (std::getline(stream, line)) {
        content << line << "\n";
    }
    return content.str();
}
static void index_handler(const std::shared_ptr<restbed::Session> session) {
    session->close(302, { { "Location", "/index.html" } });
}
static void file_handler(const std::shared_ptr<restbed::Session> session) {
    auto request = session->get_request();
    std::string filename = request->get_path_parameter("filename");
    std::ifstream stream("./site/" + filename);
    if (stream.is_open()) {
        std::string content = read_file(stream);
        stream.close();
        std::multimap<std::string, std::string> headers = {
            { "Content-Length", std::to_string(content.length()) }
        };
        session->close(200, content, headers);
    } else {
        session->close(404);
    }
}
// buggy string parsing, fix later
static std::string get_filename(const std::string& data) {
    std::string paramname = "filename";
    size_t pos = data.find(paramname);
    size_t data_begins = pos + paramname.length() + 2 /* =" */;
    size_t data_ends = data.find("\"", data_begins);
    return data.substr(data_begins, data_ends - data_begins);
}
static std::string get_gcode_data(const std::string& data) {
    size_t content_type_declaration = data.find("Content-Type");
    size_t data_begins = data.find(";", content_type_declaration);
    size_t data_ends = data.find("------", data_begins) - 2;
    return data.substr(data_begins, data_ends - data_begins);
}
static void parse_data(const std::string& data) {
    std::string filename = get_filename(data);
    std::string gcode_data = get_gcode_data(data);
    std::string output_file = "cache/" + filename;
    std::ofstream file(output_file);
    file << gcode_data;
    file.close();
}
static void upload_handler(const std::shared_ptr<restbed::Session> session) {
    auto request = session->get_request();
    size_t length = request->get_header("Content-Length", 0);
    session->fetch(length, [request](std::shared_ptr<restbed::Session> session, const restbed::Bytes& body) {
        std::string data = std::string((char*)body.data(), body.size());
        parse_data(data);
        session->close(302, { { "Location", "/success.html" } });
    });
}
static void add_file_type(const std::string& extension, const std::string& content_type, restbed::Service& service) {
    auto resource = std::make_shared<restbed::Resource>();
    resource->set_path("/{filename: [a-z]*\\." + extension + "}");
    resource->set_method_handler("GET", file_handler);
    resource->set_default_header("Content-Type", content_type);
    service.publish(resource);
}
int main(int argc, const char* argv[]) {
    auto index = std::make_shared<restbed::Resource>();
    index->set_path("/");
    index->set_method_handler("GET", index_handler);
    auto upload = std::make_shared<restbed::Resource>();
    upload->set_path("/api/upload");
    upload->set_method_handler("POST", upload_handler);
    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(PORT);
    settings->set_default_header("Connection", "close");
    restbed::Service service;
    service.publish(index);
    service.publish(upload);
    add_file_type("html", "text/html", service);
    service.start(settings);
    return 0;
}