/*
20210925 kakao 2nd coding test

<<요약>>
매칭 서버
유저들은 고유한 실력 가지고 있음
유저들의 게임 승패 및 걸린 시간을 통해 실력을 유추하여
실제 유저들의 실력 순서와 게임의 승패 기록을 기준으로 구성한 실력이 일치하도록 구현하는 것이 목표
시나리오 2는 어뷰저(일부러 지는 사람) 존재

<해결 순서>
cpprest로 서버와 통신하는 함수 작성(요구 사항에 맞춰 작동하도록 구현)-1시간 반
문제에 부합하도록 조정(데이터 가공 및 코드 흐름 구현)-1시간 반
나머지 시간-매칭 알고리즘 조정 작업 및 테스트

<발생했던 오류>
http_msg에는 유니코드 문자열만 입력 가능하다. 멀티바이트 문자열(string형)을 쓰면
직접적인 원인과 무관한 '<< 연산자 에러'가 발생하므로 주의한다. (wstring으로 해결)

<그 외...>
한번 돌려보는데 (시나리오 1,2 다) 20~30분 가량 소모 되므로 자주 돌려 보기는 어렵다.
리더보드 순위는 확인안해서 모른다...

기록/참고 용으로 업데이트는 없을 예정
*/

#include <iostream>
#include <map>
#include <queue>
#include <string>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
using namespace std;

map<string, string> Auth_Key_Map;
map<int, int> user_Grade_Map;
priority_queue<pair<int,int>> WaitingIDQ;

wstring base_url = L"https://huqeyhi95c.execute-api.ap-northeast-2.amazonaws.com/prod";

auto POST_request(int problem) {
    // Create user data as JSON object and make POST request.
    auto postJson = pplx::create_task([=]() {
        json::value jsonObject;
        jsonObject[U("problem")] = json::value::number(problem);
        http_client client(base_url+L"/start");
        http_request msg(methods::POST);
        msg.headers().add(U("X-Auth-Token"), U("e5b40901daeb2f3f2092d8d546e492a0"));
        msg.headers().add(U("Content-Type"), U("application/json"));
        msg.set_body(jsonObject.serialize());
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
        Auth_Key_Map["auth_key"] = utility::conversions::to_utf8string(jsonObject[U("auth_key")].as_string());
    });
    return postJson;
}

auto GET_request(wstring wstr) {
    // Make a GET request.
    auto requestJson = http_client(base_url+wstr);
    http_request msg(methods::GET);
    wstring uni_string_Key;
    uni_string_Key.assign(Auth_Key_Map["auth_key"].begin(), Auth_Key_Map["auth_key"].end());
    msg.headers().add(U("Authorization"), uni_string_Key);
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
        .then([=](json::value jsonObject) {
        if (wstr[1] == 's')  // score 인 경우
            return jsonObject;
        else
            return jsonObject[wstr.substr(1, wstr.size()-1)];
    })
        // Parse the user details.
        .then([=](json::value jsonObject) {
        // WaitingLine / GameResult / UserInfo / Score 각각 다르게 처리 필요
        switch (wstr[1])
        {
        case 'w':
            for (int i = 0; i < jsonObject.as_array().size(); ++i)
            {
                int id = jsonObject.as_array().at(i)[U("id")].as_integer();
                int from = jsonObject.as_array().at(i)[U("from")].as_integer();
                //std::wcout << "id: " << id << " : from: " << from << std::endl;
                WaitingIDQ.push({user_Grade_Map[id] ,id });
            }
            break;
        case 'g':
            for (int i = 0; i < jsonObject.as_array().size(); ++i)
            {
                int win = jsonObject.as_array().at(i)[U("win")].as_integer();
                int lose = jsonObject.as_array().at(i)[U("lose")].as_integer();
                int taken = jsonObject.as_array().at(i)[U("taken")].as_integer();
                //std::wcout << "win: " << win << " lose: " << lose << " taken: " << taken << std::endl;
                // 점수 조정
                // taken : 40분 - (두 유저 간 고유 실력 차 / 99000 * 35) + e
                int diff = (40 - taken) * 99000 / 35;
                while (diff >= 100)
                {
                    diff /= 10;
                }
                if (user_Grade_Map[win] + diff > 9999)
                    user_Grade_Map[win] = 9999;
                else
                    user_Grade_Map[win] += diff;
                if (user_Grade_Map[lose] - diff > 0)
                    user_Grade_Map[lose] - diff;
                else
                    user_Grade_Map[lose] = 0;
            }
            break;
        case 'u':
            for (int i = 0; i < jsonObject.as_array().size(); ++i)
            {
                int id = jsonObject.as_array().at(i)[U("id")].as_integer();
                int grade = jsonObject.as_array().at(i)[U("grade")].as_integer();
                //std::wcout << "id: " << id << " grade: " << grade << std::endl;   // 0초기화 상태임
                user_Grade_Map[id] = grade;
            }
            break;
        case 's':
            std::wcout << "status: " << jsonObject[U("status")].as_string() << endl;
            std::wcout << "efficiency_score: " << jsonObject[U("efficiency_score")].as_string() << endl;
            std::wcout << "accuracy_score1: " << jsonObject[U("accuracy_score1")].as_string() << endl;
            std::wcout << "accuracy_score2: " << jsonObject[U("accuracy_score2")].as_string() << endl;
            std::wcout << "score: " << jsonObject[U("score")].as_string() << endl;
            break;
        }
    });
}

auto MatchAPI(vector<vector<json::value>>& jsonvect) {
    auto putJson = http_client(base_url + L"/match");
    json::value jsonObject;
    jsonObject[U("pairs")] = json::value::array();
    for (int i = 0; i < jsonvect.size(); ++i)
    {
        jsonObject[U("pairs")][i] = json::value::array(jsonvect[i]);
    }
    http_request msg(methods::PUT);
    wstring uni_string_Key;
    uni_string_Key.assign(Auth_Key_Map["auth_key"].begin(), Auth_Key_Map["auth_key"].end());
    msg.headers().add(U("Authorization"), uni_string_Key);
    msg.headers().add(U("Content-Type"), U("application/json"));
    msg.set_body(jsonObject.serialize());
    return putJson.request(msg)

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
        wcout << "MatchAPI:: " << "status: " << jsonObject[U("status")].as_string() << " time: " << jsonObject[U("time")].as_integer() << endl;
    });
}

auto ChangeGradeAPI(vector<json::value>& jsonvect) {
    auto putJson = http_client(base_url + L"/change_grade");
    json::value jsonObject;
    jsonObject[U("commands")] = json::value::array();
    for (int i = 0; i < jsonvect.size(); ++i)
    {   // json 입력 예시: { "id": 1, "grade": 1900 }
        jsonObject[U("commands")][i] = jsonvect[i];
    }
    http_request msg(methods::PUT);
    wstring uni_string_Key;
    uni_string_Key.assign(Auth_Key_Map["auth_key"].begin(), Auth_Key_Map["auth_key"].end());
    msg.headers().add(U("Authorization"), uni_string_Key);
    msg.headers().add(U("Content-Type"), U("application/json"));
    msg.set_body(jsonObject.serialize());
    return putJson.request(msg)

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
        //wcout << "ChangeGradeAPI:: " << "status: " << jsonObject[U("status")].as_string() << endl;
    });
}

int main()
{
    for (int problem = 2; problem <= 2; ++problem)
    {
        auto StartAPI = POST_request(problem);
        // Wait for the concurrent tasks to finish.
        try {
            StartAPI.wait();    // auth key 받기
            cout << Auth_Key_Map["auth_key"] << endl;
            auto UserInfoAPI = GET_request(L"/user_info").wait(); // 최초 1회만 받고 이후론 map에서 update
            vector<vector<json::value>> jsonvect(0);
            for (int m = 0; m < 595; ++m)
            {
                auto WaitingLineAPI = GET_request(L"/waiting_line").wait();
                while (!WaitingIDQ.empty())
                {
                    int id1 = WaitingIDQ.top().second; WaitingIDQ.pop();
                    if (WaitingIDQ.empty())
                        break;
                    int id2 = WaitingIDQ.top().second; WaitingIDQ.pop();

                    vector<json::value> jsvect = { json::value::number(id1), json::value::number(id2) };
                    jsonvect.push_back(jsvect);
                }
                MatchAPI(jsonvect).wait();
                jsonvect.clear();
                auto GameResultAPI = GET_request(L"/game_result");
                json::value jsonObject;
                vector<json::value> jsvect;
                for (auto& e : user_Grade_Map)
                {
                    jsonObject[U("id")] = json::value::number(e.first);
                    jsonObject[U("grade")] = json::value::number(e.second);
                    jsvect.emplace_back(jsonObject);
                }
                ChangeGradeAPI(jsvect).wait();
            }
            MatchAPI(jsonvect).wait();  // server finish

            auto GetScoreAPI = GET_request(L"/score");
            GetScoreAPI.wait();
        }
        catch (const std::exception& e) {
            printf("Error exception:%s\n", e.what());
        }
        user_Grade_Map.clear();
    }

	return 0;
}