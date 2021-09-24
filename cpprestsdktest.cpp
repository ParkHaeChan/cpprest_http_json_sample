/*
* 참고
* http://www.atakansarioglu.com/easy-quick-start-cplusplus-rest-client-example-cpprest-tutorial/
* 
* kakao 코테 2차 대비하여 준비해보았다.
* 예제 코드는 프로그래머스에서 연습해 볼 수 있는 2021 2차를 기준으로 작동을 확인
* 
* 문제는 못 풀어봤고, 그냥 연결만 잘 되는 것 확인
* 카카오 2021 2차 : https://programmers.co.kr/skill_check_assignments/67
* 
* 문제에 사용된 auth 토큰은 계속 바뀐다. (응시 할때 마다 새로 발급)
* 
* 아직 모르는 기능도 많고해서 일단 참고한 코드의 틀을 대체로 유지함
* 
* pplx: 병렬처리 기능 namespace이다. task를 선언할 때 사용된다.
* main 에서는 task를 함수에서 실행후 반환하는 식으로 구현한다.
* 이때 try-catch 문 내에서 wait()를 호출하여 response를 받을 때까지 기다려야 프로그램이 바로 종료되지 않는다.
* 
* 기본적인 GET, POST, PUT 등은 아래 코드를 참고하면 된다.
* http 해더 사용법이 없어서 다른 곳에서 참고하여 추가하였다. (이거 할 줄 모르면 401에러가 발생해도 해결이 안된다)
* 아래 msg.headers().add()를 참고
* 
* json은 stl의 map처럼 관리된다. 
* 서버 응답이 json형식이면 .then([=](http_response response) 에서 response.extract_json()으로 받으면 된다.
* .then([](json::value jsonObject) { ... 로 이어서 처리할 수 있다.
* 이때 jsonOject를 map처럼 사용할 수 있다.
* 그전에 json  형식이 배열이거나, 각종 자료형일 정우 as_XXXX 메서드를 이용해서 변환하여 처리하여야한다.
* 그냥 jsonObject 전체를 문자열로 확인하려면 .serialize()를 호출한다.
* U("xxx") == L"xxx"로 유니코드 문자열 표시이다. (L"유니코드문자열" 사용시 추가 필요: #include <string> using namespace std::string_literals;)
* 
* uri_builder: uri구성을 좀 더 편하게 해주려고 쓰이는데 안써도 상관없다. (base_url 뒤에 추가되는 방식으로 사용되는 듯)
* 
* request_msg body로 json 추가하기 (POST 함수 참고)
* json::value jsonObject 로 객체 생성 후 map처럼 사용하여 key:value 를 저장시킬 수 있다.
* msg.set_body(jsonObject.serialize());로 body에 추가
* 
*/

#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

auto GET_print_to_json() {
    // Create a file stream to write the received file into it.
    auto fileStream = std::make_shared<ostream>();

    // Open stream to output file.
    pplx::task<void> requestTask = fstream::open_ostream(U("users.json"))

        // Make a GET request.
        .then([=](ostream outFile) {
        *fileStream = outFile;

        // Create http_client to send the request.
        http_client client(U("https://reqres.in"));

        // Build request URI and start the request.
        return client.request(methods::GET, uri_builder(U("api")).append_path(U("users")).to_string());
    })

        // Get the response.
        .then([=](http_response response) {
        // Check the status code.
        if (response.status_code() != 200) {
            throw std::runtime_error("Returned " + std::to_string(response.status_code()));
        }

        // Write the response body to file stream.
        response.body().read_to_end(fileStream->streambuf()).wait();
        std::wcout << "\n\n========json 출력 완료=========\n\n"
            << response.body() << std::endl;
        // Close the file.
        return fileStream->close();
    });

    return requestTask;
}

auto GET_request() {
    // Make a GET request.
    auto requestJson = http_client(U("https://kox947ka1a.execute-api.ap-northeast-2.amazonaws.com/prod/users/locations"));
    http_request msg(methods::GET);
    msg.headers().add(U("Authorization"), U("4851219a-b18c-479f-b729-c9acc8fb2758"));
    msg.headers().add(U("Content-Type"), U("application/json"));
    return requestJson.request(msg)

    // Get the response.
    .then([](http_response response) {
    // Check the status code.
    if (response.status_code() != 200) {
        throw std::runtime_error("Returned " + std::to_string(response.status_code()));
    }

    // Convert the response body to JSON object.
    return response.extract_json();
    })

    // Get the data field.
    .then([](json::value jsonObject) {
    return jsonObject[U("locations")];
    })
    // Parse the user details.
    .then([](json::value jsonObject) {
    //std::wcout << "\n==Get request 확인==\n" << jsonObject.serialize() << std::endl;
    //<< "id:" << jsonObject[U("id")].as_integer()
    //<< "located_bikes_count" << jsonObject[U("located_bikes_count")].as_integer()

    // json에 값으로 배열이 있는 경우: as_array()와  at(idx)으로 배열처럼 접근 가능 
    for (int i = 0; i < jsonObject.as_array().size(); ++i)
    {
        //std::wcout << jsonObject.as_array().at(i).serialize() << std::endl;
        std::wcout << jsonObject.as_array().at(i)[U("id")] << ":" << jsonObject.as_array().at(i)[U("located_bikes_count")] << std::endl;
    }

    });
}

auto POST_request() {
    // Create user data as JSON object and make POST request.
    auto postJson = pplx::create_task([]() {
        json::value jsonObject;
        jsonObject[U("problem")] = json::value::number(1);
        http_client client(U("https://kox947ka1a.execute-api.ap-northeast-2.amazonaws.com/prod/users/start"));
        http_request msg(methods::POST);
        msg.headers().add(U("X-Auth-Token"), U("6ed1cbf80435532a7761de666e9b565e"));
        msg.headers().add(U("Content-Type"), U("application/json"));
        msg.set_body(jsonObject.serialize());
        std::wcout << jsonObject.serialize() << std::endl;
        return client.request(msg);
    })
        // Get the response.
        .then([](http_response response) {
        // Check the status code.
        if (response.status_code() != 200) {
            throw std::runtime_error("Returned " + std::to_string(response.status_code()));
        }

        // Convert the response body to JSON object.
        return response.extract_json();
    })

        // Parse the user details.
        .then([](json::value jsonObject) {
        std::wcout << "\n==POST request 확인==\n"
            //<< jsonObject[U("auth_key")].as_string()
            //<< " " << jsonObject[U("problem")].as_integer()
            //<< " (" << jsonObject[U("time")].as_integer() << ") \n"
            << jsonObject.serialize()
            << std::endl;
    });

    return postJson;
}

auto PUT_request() {
    // Make PUT request with {"name": "atakan", "location": "istanbul"} data.
    auto putJson = http_client(U("https://reqres.in"))
        .request(methods::PUT,
            uri_builder(U("api")).append_path(U("users")).append_path(U("1")).to_string(),
            U("{\"name\": \"atakan\", \"location\": \"istanbul\"}"),
            U("application/json"))

        // Get the response.
        .then([](http_response response) {
        if (response.status_code() != 200) {
            throw std::runtime_error("Returned " + std::to_string(response.status_code()));
        }

        // Convert the response body to JSON object.
        return response.extract_json();
    })

        // Parse the user details.
        .then([](json::value jsonObject) {
        std::wcout << "\n==PUT request 확인==\n"
            << jsonObject[U("name")].as_string()
            << "    " << jsonObject[U("location")].as_string() << "  \n"
            << jsonObject.serialize()
            << std::endl;
    });

    return putJson;
}

auto PATCH_request() {
    // Make PATCH request with {"name": "sarioglu"} data.
    auto patchJson = http_client(U("https://reqres.in"))
        .request(methods::PATCH,
            uri_builder(U("api")).append_path(U("users")).append_path(U("1")).to_string(),
            U("{\"name\": \"sarioglu\"}"),
            U("application/json"))

        // Get the response.
        .then([](http_response response) {
        if (response.status_code() != 200) {
            throw std::runtime_error("Returned " + std::to_string(response.status_code()));
        }

        // Print the response body.
        std::wcout << "\n==PATCH request 확인==\n"
            << response.extract_json().get().serialize() << "  1 \n"
            //<< response.extract_json().get().to_string() << "  2 \n"
            << response.to_string()
            << std::endl;
    });

    return patchJson;
}

auto DEL_request() {
    // Make DEL request.
    auto deleteJson = http_client(U("https://reqres.in"))
        .request(methods::DEL,
            uri_builder(U("api")).append_path(U("users")).append_path(U("1")).to_string())
        // Get the response.
        .then([](http_response response) {
        std::wcout << "\n==DEL request 확인==\n"
            << "Deleted: " << std::boolalpha << (response.status_code() == 204) << "\n"
            << response.to_string()
            << std::endl;
    });

    return deleteJson;
}

void GetJson()
{
    http_client client(U("https://jsonplaceholder.typicode.com/users"));

    http_request req(methods::GET);

    client.request(req).then([=](http_response r) {
        std::wcout << U("STATUS : ") << r.status_code() << std::endl;
        std::wcout << "content-type : " << r.headers().content_type() << std::endl;

        //{
        //		"time": "11:25:23 AM",
        //		"milliseconds_since_epoch" : 1423999523092,
        //		"date" : "02-15-2015"
        //}

        r.extract_json(true).then([](json::value v) {
            //  std::wcout << v.serialize() << std::endl; (json value를 문자열로 변환)
            std::wcout << v.as_array()[0].at(U("id")) << std::endl;
            std::wcout << v.as_array()[0].at(U("name")) << std::endl;
            std::wcout << v.as_array()[0].at(U("email")) << std::endl;
        }).wait();

    }).wait();

}

int main() {

    //auto postJson = POST_request();
    auto getJson = GET_request();
    // Wait for the concurrent tasks to finish.
    try {
        //postJson.wait();
        getJson.wait();
    }
    catch (const std::exception& e) {
        printf("Error exception:%s\n", e.what());
    }

    return 0;
}