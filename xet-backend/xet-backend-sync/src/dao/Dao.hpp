#ifndef Dao_HPP
#define Dao_HPP

#include "dto/DTOs.hpp"
#include <mysqlx/xdevapi.h>
#include "oatpp/core/macro/codegen.hpp"
#include <iostream>
#include "../LivingstreamComponent.hpp"
#include "../MySQLComponent.hpp"
extern mysqlx::Client cli;

class Dao
{
public:
  Dao() = default;
  // 获取用户信息(不包括隐私信息,如邮箱和密码)
  oatpp::Object<RUserDto> getUserById(const oatpp::Int64 &id)
  {
    auto DBSession = cli.getSession();

    DBSession.sql("USE xet_living_table").execute();

    auto result_db = DBSession.sql("SELECT user_id,username,avatar_url FROM users WHERE user_id = ?").bind((int64_t)id).execute();

    if (result_db.count() == 0)
    {
      std::cerr << "No user found with id: " << id << std::endl;
      return nullptr;
    }

    auto row = result_db.fetchOne();
    auto result_dto = RUserDto::createShared();
    // 在oatpp中,所有的元素都需要初始化,否则分分钟段错误
    result_dto->data = oatpp::List<oatpp::Object<UserDto>>::createShared();

    result_dto->statusCode = 200;
    result_dto->message = "Find user ok";

    auto userdto = UserDto::createShared();
    userdto->id = row[0].get<int64_t>();
    if (!row[1].isNull())
      userdto->name = row[1].get<std::string>();
    else
      userdto->name = "";
    if (!row[2].isNull())
      userdto->avatar_url = row[2].get<std::string>();
    else
      userdto->avatar_url = "";
    result_dto->data->push_back(userdto);

    return result_dto;
  }
  // 第一个参数是用户id
  oatpp::Object<RHomeDto> getHomeById(const oatpp::Int64 &id)
  {
    auto DBSession = cli.getSession();
    auto result_dto = RHomeDto::createShared();
    result_dto->data = oatpp::List<oatpp::Object<LiveDto>>::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    // 查询该用户拥有的所有直播id
    auto result_streams = DBSession.sql(
                                       "SELECT living_stream_id FROM user_living_stream WHERE user_id = ?")
                              .bind((int64_t)id)
                              .execute();

    // 如果没有直播
    if (result_streams.count() == 0)
    {
      result_dto->statusCode = 404;
      result_dto->message = "No living streams found for the user.";
      return result_dto;
    }

    // 遍历每个直播
    for (auto stream_row : result_streams)
    {
      int64_t living_stream_id = stream_row[0].get<int64_t>();

      // 查询直播详细信息
      auto result_live = DBSession.sql(
                                      "SELECT living_stream_id, creator_user_id, description, living_cover_url, living_title FROM living_stream WHERE living_stream_id = ?")
                             .bind(living_stream_id)
                             .execute();

      if (result_live.count() == 0)
        continue;

      auto live_row = result_live.fetchOne();
      auto livedto = LiveDto::createShared();
      livedto->living_stream_id = live_row[0].get<int64_t>();
      livedto->creator_user_id = live_row[1].get<int64_t>();
      if (!live_row[2].isNull())
        livedto->description = live_row[2].get<std::string>();
      else
        livedto->description = "";
      if (!live_row[3].isNull())
        livedto->living_cover_url = live_row[3].get<std::string>();
      else
        livedto->living_cover_url = "";
      if (!live_row[4].isNull())
        livedto->living_title = live_row[4].get<std::string>();
      else
        livedto->living_title = "";
      // 查询该直播的所有回放
      livedto->playbacks = oatpp::List<oatpp::Object<LivePlaybackDto>>::createShared();
      auto result_playbacks = DBSession.sql(
                                           "SELECT playback_id, playback_title, DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, playback_url "
                                           "FROM live_playback WHERE living_stream_id = ? ORDER BY created_at DESC")
                                  .bind(living_stream_id)
                                  .execute();

      for (auto playback_row : result_playbacks)
      {
        auto playbackdto = LivePlaybackDto::createShared();
        playbackdto->playback_id = playback_row[0].get<int64_t>();
        if (!playback_row[1].isNull())
          playbackdto->playback_title = playback_row[1].get<std::string>();
        else
          playbackdto->playback_title = "";
        if (!playback_row[2].isNull())
          playbackdto->created_at = playback_row[2].get<std::string>();
        else
          playbackdto->created_at = "";
        if (!playback_row[3].isNull())
          playbackdto->playback_url = playback_row[3].get<std::string>();
        else
          playbackdto->playback_url = "";
        livedto->playbacks->push_back(playbackdto);
      }

      result_dto->data->push_back(livedto);
    }

    result_dto->statusCode = 200;
    result_dto->message = "Find home info ok";
    return result_dto;
  }

  // 第一个参数是直播id,第二个参数是初始化的时候评论,讲解等的最大数目
  oatpp::Object<RLiveDto> getLiveById(const oatpp::Int64 &id, const int64_t pagesize)
  {
    auto DBSession = cli.getSession();
    auto result_dto = RLiveDto::createShared();
    result_dto->data = oatpp::List<oatpp::Object<LiveDto>>::createShared();
    auto livedto = LiveDto::createShared();
    result_dto->data->push_back(livedto);

    // 初始化 comments, explanations, files 列表
    livedto->comments = oatpp::List<oatpp::Object<LiveCommentDto>>::createShared();
    livedto->explanations = oatpp::List<oatpp::Object<LiveExplaDto>>::createShared();
    livedto->files = oatpp::List<oatpp::Object<LiveFileDto>>::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    // 查询直播信息
    auto result_live_db = DBSession.sql("SELECT living_stream_id,creator_user_id,description,living_stream_url,living_comment_room_url,living_expla_room_url,living_broadcast_room_url,isliving,living_url,living_stream_code FROM living_stream WHERE living_stream_id = ?")
                              .bind((int64_t)id)
                              .execute();

    if (result_live_db.count() == 0)
    {
      result_dto->statusCode = 404;
      result_dto->message = "No living stream found with the given ID.";
      return result_dto;
    }

    auto live_row = result_live_db.fetchOne();

    result_dto->statusCode = 200;
    result_dto->message = "Find living_stream infomation ok";

    livedto->living_stream_id = live_row[0].get<int64_t>();
    livedto->creator_user_id = live_row[1].get<int64_t>();
    if (!live_row[2].isNull())
      livedto->description = live_row[2].get<std::string>();
    else
      livedto->description = "";
    if (!live_row[3].isNull())
      livedto->living_stream_url = live_row[3].get<std::string>();
    else
      livedto->living_stream_url = "";
    if (!live_row[4].isNull())
      livedto->living_comment_room_url = live_row[4].get<std::string>();
    else
      livedto->living_comment_room_url = "";
    if (!live_row[5].isNull())
      livedto->living_expla_room_url = live_row[5].get<std::string>();
    else
      livedto->living_expla_room_url = "";
    if (!live_row[6].isNull())
      livedto->living_broadcast_room_url = live_row[6].get<std::string>();
    else
      livedto->living_broadcast_room_url = "";
    livedto->isliving = live_row[7].get<bool>();
    if (!live_row[8].isNull())
      livedto->living_url = live_row[8].get<std::string>();
    else
      livedto->living_url = "";
    if (!live_row[9].isNull())
      livedto->living_stream_code = live_row[9].get<std::string>();
    else
      livedto->living_stream_code = "";
    // 查询评论总数并分页
    auto total_comments_result = DBSession.sql("SELECT COUNT(*) FROM live_comment WHERE living_stream_id = ?").bind((int64_t)id).execute();
    auto total_comments_count = total_comments_result.fetchOne()[0].get<int64_t>();
    livedto->page_count_comment = (total_comments_count % pagesize) == 0 ? (total_comments_count / pagesize) : (total_comments_count / pagesize) + 1;
    int64_t comments_offset = (total_comments_count > pagesize) ? total_comments_count - pagesize : 0;

    auto result_comments_db = DBSession.sql("SELECT comment_id,living_stream_id,creator_user_id,DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at,content FROM live_comment WHERE living_stream_id = ? ORDER BY created_at LIMIT ? OFFSET ?").bind((int64_t)id).bind((int64_t)pagesize).bind(comments_offset).execute();

    for (auto live_comments_row : result_comments_db)
    {
      auto comment = LiveCommentDto::createShared();
      comment->comment_id = live_comments_row[0].get<int64_t>();
      comment->living_stream_id = live_comments_row[1].get<int64_t>();
      comment->creator_user_id = live_comments_row[2].get<int64_t>();
      if (!live_comments_row[3].isNull())
        comment->created_at = live_comments_row[3].get<std::string>();
      else
        comment->created_at = "";
      if (!live_comments_row[4].isNull())
        comment->content = live_comments_row[4].get<std::string>();
      else
        comment->content = "";
      livedto->comments->push_back(comment);
    }

    // 查询讲解总数并分页
    auto total_explanations_result = DBSession.sql("SELECT COUNT(*) FROM live_explanation WHERE living_stream_id = ?").bind((int64_t)id).execute();
    auto total_explanations_count = total_explanations_result.fetchOne()[0].get<int64_t>();
    livedto->page_count_explanation = (total_explanations_count % pagesize) == 0 ? (total_explanations_count / pagesize) : (total_explanations_count / pagesize) + 1;
    int64_t explanations_offset = (total_explanations_count > pagesize) ? total_explanations_count - pagesize : 0;

    auto result_explanations_db = DBSession.sql("SELECT expla_id,living_stream_id,creator_user_id,DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at,content FROM live_explanation WHERE living_stream_id = ? ORDER BY created_at LIMIT ? OFFSET ?").bind((int64_t)id).bind((int64_t)pagesize).bind(explanations_offset).execute();

    for (auto explanation_row : result_explanations_db)
    {
      auto explanation = LiveExplaDto::createShared();
      explanation->expla_id = explanation_row[0].get<int64_t>();
      explanation->living_stream_id = explanation_row[1].get<int64_t>();
      explanation->creator_user_id = explanation_row[2].get<int64_t>();
      if (!explanation_row[3].isNull())
        explanation->created_at = explanation_row[3].get<std::string>();
      else
        explanation->created_at = "";
      if (!explanation_row[4].isNull())
        explanation->content = explanation_row[4].get<std::string>();
      else
        explanation->content = "";
      livedto->explanations->push_back(explanation);
    }

    // 查询文件总数并分页
    auto total_files_result = DBSession.sql("SELECT COUNT(*) FROM live_file WHERE living_stream_id = ?").bind((int64_t)id).execute();
    auto total_files_count = total_files_result.fetchOne()[0].get<int64_t>();
    livedto->page_count_file = (total_files_count % pagesize) == 0 ? (total_files_count / pagesize) : (total_files_count / pagesize) + 1;
    int64_t files_offset = (total_files_count > pagesize) ? total_files_count - pagesize : 0;

    auto result_files_db = DBSession.sql("SELECT file_id,living_stream_id,creator_user_id,DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at,file_url FROM live_file WHERE living_stream_id = ? ORDER BY created_at LIMIT ? OFFSET ?").bind((int64_t)id).bind((int64_t)pagesize).bind(files_offset).execute();

    for (auto file_row : result_files_db)
    {
      auto file = LiveFileDto::createShared();
      file->file_id = file_row[0].get<int64_t>();
      file->living_stream_id = file_row[1].get<int64_t>();
      file->creator_user_id = file_row[2].get<int64_t>();
      if (!file_row[3].isNull())
        file->created_at = file_row[3].get<std::string>();
      else
        file->created_at = "";
      if (!file_row[4].isNull())
        file->file_url = file_row[4].get<std::string>();
      else
        file->file_url = "";
      livedto->files->push_back(file);
    }

    return result_dto;
  }

  oatpp::Object<RLivePlaybackDto> getLivePlaybackById(const oatpp::Int64 &id)
  {
    auto DBSession = cli.getSession();

    DBSession.sql("USE xet_living_table").execute();

    auto result_db = DBSession.sql("SELECT playback_id,playback_title,DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at,playback_url FROM live_playback WHERE playback_id = ?").bind((int64_t)id).execute();

    if (result_db.count() == 0)
    {
      std::cerr << "No playback found with id: " << id << std::endl;
      return nullptr;
    }

    auto row = result_db.fetchOne();
    auto result_dto = RLivePlaybackDto::createShared();
    // 在oatpp中,所有的元素都需要初始化,否则分分钟段错误
    result_dto->data = oatpp::List<oatpp::Object<LivePlaybackDto>>::createShared();

    result_dto->statusCode = 200;
    result_dto->message = "Find playback ok";

    auto playbackdto = LivePlaybackDto::createShared();
    playbackdto->playback_id = row[0].get<int64_t>();
    if (!row[1].isNull())
      playbackdto->playback_title = row[1].get<std::string>();
    else
      playbackdto->playback_title = "";
    if (!row[2].isNull())
      playbackdto->created_at = row[2].get<std::string>();
    else
      playbackdto->created_at = "";
    if (!row[3].isNull())
      playbackdto->playback_url = row[3].get<std::string>();
    else
      playbackdto->playback_url = "";
    result_dto->data->push_back(playbackdto);

    return result_dto;
  }

  // 第一个参数是直播id,第二个参数是页面大小,第三个是面数
  oatpp::Object<RLiveCommentDto> getLiveComment(const oatpp::Int64 &id, const int64_t pagesize, int64_t page)
  {
    auto DBSession = cli.getSession();
    auto result_dto = RLiveCommentDto::createShared();
    result_dto->data = oatpp::List<oatpp::Object<LiveCommentDto>>::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    // 查询评论总数
    auto total_comments_result = DBSession.sql("SELECT COUNT(*) FROM live_comment WHERE living_stream_id = ?")
                                     .bind((int64_t)id)
                                     .execute();
    auto total_comments_count = total_comments_result.fetchOne()[0].get<int64_t>();

    // 计算偏移量
    int64_t comments_offset = (page - 1) * pagesize;

    // 检查偏移量是否超出范围
    if (comments_offset >= total_comments_count)
    {
      result_dto->statusCode = 404;
      result_dto->message = "No comments found for the given page.";
      return result_dto;
    }

    // 查询评论数据
    auto result_comments_db = DBSession.sql("SELECT comment_id, living_stream_id, creator_user_id, DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, content FROM live_comment WHERE living_stream_id = ? ORDER BY created_at LIMIT ? OFFSET ?")
                                  .bind((int64_t)id)
                                  .bind((int64_t)pagesize)
                                  .bind(comments_offset)
                                  .execute();

    for (auto live_comments_row : result_comments_db)
    {
      auto comment = LiveCommentDto::createShared();
      comment->comment_id = live_comments_row[0].get<int64_t>();
      comment->living_stream_id = live_comments_row[1].get<int64_t>();
      comment->creator_user_id = live_comments_row[2].get<int64_t>();
      if (!live_comments_row[3].isNull())
        comment->created_at = live_comments_row[3].get<std::string>();
      else
        comment->created_at = "";
      if (!live_comments_row[4].isNull())
        comment->content = live_comments_row[4].get<std::string>();
      else
        comment->content = "";
      result_dto->data->push_back(comment);
    }

    result_dto->statusCode = 200;
    result_dto->message = "Comments retrieved successfully.";
    return result_dto;
  }

  // 第一个参数是直播id,第二个参数是页面大小,第三个是面数
  oatpp::Object<RLiveExplaDto> getLiveExpla(const oatpp::Int64 &id, const int64_t pagesize, int64_t page)
  {
    auto DBSession = cli.getSession();
    auto result_dto = RLiveExplaDto::createShared();
    result_dto->data = oatpp::List<oatpp::Object<LiveExplaDto>>::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    // 查询讲解总数
    auto total_explanations_result = DBSession.sql("SELECT COUNT(*) FROM live_explanation WHERE living_stream_id = ?")
                                         .bind((int64_t)id)
                                         .execute();
    auto total_explanations_count = total_explanations_result.fetchOne()[0].get<int64_t>();

    // 计算偏移量
    int64_t explanations_offset = (page - 1) * pagesize;

    // 检查偏移量是否超出范围
    if (explanations_offset >= total_explanations_count)
    {
      result_dto->statusCode = 404;
      result_dto->message = "No explanations found for the given page.";
      return result_dto;
    }

    // 查询讲解数据
    auto result_explanations_db = DBSession.sql("SELECT expla_id, living_stream_id, creator_user_id, DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, content FROM live_explanation WHERE living_stream_id = ? ORDER BY created_at LIMIT ? OFFSET ?")
                                      .bind((int64_t)id)
                                      .bind((int64_t)pagesize)
                                      .bind(explanations_offset)
                                      .execute();

    for (auto explanation_row : result_explanations_db)
    {
      auto explanation = LiveExplaDto::createShared();
      explanation->expla_id = explanation_row[0].get<int64_t>();
      explanation->living_stream_id = explanation_row[1].get<int64_t>();
      explanation->creator_user_id = explanation_row[2].get<int64_t>();
      if (!explanation_row[3].isNull())
        explanation->created_at = explanation_row[3].get<std::string>();
      else
        explanation->created_at = "";
      if (!explanation_row[4].isNull())
        explanation->content = explanation_row[4].get<std::string>();
      else
        explanation->content = "";
      result_dto->data->push_back(explanation);
    }
    result_dto->statusCode = 200;
    result_dto->message = "Explanations retrieved successfully.";
    return result_dto;
  }

  // 第一个参数是直播id,第二个参数是页面大小,第三个是面数
  oatpp::Object<RLiveFileDto> getLiveFile(const oatpp::Int64 &id, const int64_t pagesize, int64_t page)
  {
    auto DBSession = cli.getSession();
    auto result_dto = RLiveFileDto::createShared();
    result_dto->data = oatpp::List<oatpp::Object<LiveFileDto>>::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    // 查询文件总数
    auto total_files_result = DBSession.sql("SELECT COUNT(*) FROM live_file WHERE living_stream_id = ?")
                                  .bind((int64_t)id)
                                  .execute();
    auto total_files_count = total_files_result.fetchOne()[0].get<int64_t>();

    // 计算偏移量
    int64_t files_offset = (page - 1) * pagesize;

    // 检查偏移量是否超出范围
    if (files_offset >= total_files_count)
    {
      result_dto->statusCode = 404;
      result_dto->message = "No files found for the given page.";
      return result_dto;
    }

    // 查询文件数据
    auto result_files_db = DBSession.sql("SELECT file_id, living_stream_id, creator_user_id, DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, file_url FROM live_file WHERE living_stream_id = ? ORDER BY created_at LIMIT ? OFFSET ?")
                               .bind((int64_t)id)
                               .bind((int64_t)pagesize)
                               .bind(files_offset)
                               .execute();

    for (auto file_row : result_files_db)
    {
      auto file = LiveFileDto::createShared();
      file->file_id = file_row[0].get<int64_t>();
      file->living_stream_id = file_row[1].get<int64_t>();
      file->creator_user_id = file_row[2].get<int64_t>();
      if (!file_row[3].isNull())
        file->created_at = file_row[3].get<std::string>();
      else
        file->created_at = "";
      if (!file_row[4].isNull())
        file->file_url = file_row[4].get<std::string>();
      else
        file->file_url = "";
      result_dto->data->push_back(file);
    }

    result_dto->statusCode = 200;
    result_dto->message = "Files retrieved successfully.";
    return result_dto;
  }

  oatpp::Object<MessageDto> instertLiveComment(oatpp::Object<LiveCommentDto> comment)
  {
    auto DBSession = cli.getSession();
    auto result_dto = MessageDto::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    try
    {
      // 插入评论数据
      DBSession.sql("INSERT INTO live_comment (living_stream_id, creator_user_id, created_at, content) VALUES (?, ?, ?, ?)")
          .bind((int64_t)comment->living_stream_id)
          .bind((int64_t)comment->creator_user_id)
          .bind((std::string)comment->created_at)
          .bind((std::string)comment->content)
          .execute();

      // 设置返回结果
      result_dto->statusCode = 200;
      result_dto->message = "Comment inserted successfully.";
    }
    catch (const std::exception &e)
    {
      // 捕获异常并设置错误信息
      result_dto->statusCode = 500;
      result_dto->message = std::string("Failed to insert comment: ") + e.what();
    }

    return result_dto;
  }

  oatpp::Object<MessageDto> instertLiveExpla(oatpp::Object<LiveExplaDto> explanation)
  {
    auto DBSession = cli.getSession();
    auto result_dto = MessageDto::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    try
    {
      // 插入讲解数据
      DBSession.sql("INSERT INTO live_explanation (living_stream_id, creator_user_id, created_at, content) VALUES (?, ?, ?, ?)")
          .bind((int64_t)explanation->living_stream_id)
          .bind((int64_t)explanation->creator_user_id)
          .bind((std::string)explanation->created_at)
          .bind((std::string)explanation->content)
          .execute();

      // 设置返回结果
      result_dto->statusCode = 200;
      result_dto->message = "Explanation inserted successfully.";
    }
    catch (const std::exception &e)
    {
      // 捕获异常并设置错误信息
      result_dto->statusCode = 500;
      result_dto->message = std::string("Failed to insert explanation: ") + e.what();
    }

    return result_dto;
  }

  oatpp::Object<MessageDto> instertLiveFile(oatpp::Object<LiveFileDto> file)
  {
    auto DBSession = cli.getSession();
    auto result_dto = MessageDto::createShared();

    // 切换到目标数据库
    DBSession.sql("USE xet_living_table").execute();

    try
    {
      // 插入文件数据
      DBSession.sql("INSERT INTO live_file (living_stream_id, creator_user_id, created_at, file_url) VALUES (?, ?, ?, ?)")
          .bind((int64_t)file->living_stream_id)
          .bind((int64_t)file->creator_user_id)
          .bind((std::string)file->created_at)
          .bind((std::string)file->file_url)
          .execute();

      // 设置返回结果
      result_dto->statusCode = 200;
      result_dto->message = "File inserted successfully.";
    }
    catch (const std::exception &e)
    {
      // 捕获异常并设置错误信息
      result_dto->statusCode = 500;
      result_dto->message = std::string("Failed to insert file: ") + e.what();
    }

    return result_dto;
  }

  oatpp::Object<RStartLivingDto> startLivingById(const oatpp::Int64 &id)
  {
    auto DBSession = cli.getSession();
    auto result_dto = RStartLivingDto::createShared();
    result_dto->data = oatpp::List<oatpp::Object<StartLivingDto>>::createShared();

    DBSession.sql("USE xet_living_table").execute();

    // 检查直播是否是开始状态
    auto check_isliving = DBSession.sql("SELECT isliving FROM living_stream where living_stream_id=?")
                              .bind((int64_t)id)
                              .execute();
    auto check_isliving_row = *(check_isliving.begin());
    bool isliving = check_isliving_row[0].get<bool>();

    if (isliving)
    {
      result_dto->statusCode = 500;
      result_dto->message = "Currently in live status!";
      return result_dto;
    }

    // if(check_ffmpeg(id)){
    //   result_dto->statusCode = 500;
    //   result_dto->message = "RTMP stream not detected!";
    //   return result_dto;
    // }

    // 构造playback_title
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_now, &now_time);
#else
    localtime_r(&now_time, &tm_now);
#endif
    char time_buf[32];
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d%H%M%S", &tm_now);
    std::string playback_title = "living_" + std::to_string(id) + time_buf;

    // 插入live_playback表
    DBSession.sql("INSERT INTO live_playback (living_stream_id, playback_title , playback_url) VALUES (?, ? ,' ')")
        .bind((int64_t)id)
        .bind(playback_title)

        .execute();

    // 获取刚插入的playback_id
    auto result = DBSession.sql("SELECT LAST_INSERT_ID()").execute();
    int64_t playback_id = result.fetchOne()[0].get<int64_t>();

    // 构造living_stream_url, living_stream_code, living_url
    std::string living_stream_url = RTMP_URL;
    std::string living_stream_code = std::to_string(id);
    std::string living_url = FILE_URL + std::to_string(playback_id) + "/playback" + std::to_string(playback_id) + ".m3u8";

    // 更新living_stream表
    DBSession.sql("UPDATE living_stream SET living_stream_url = ?, living_stream_code = ?, living_url = ? , isliving =1 WHERE living_stream_id = ?")
        .bind(living_stream_url)
        .bind(living_stream_code)
        .bind(living_url)
        .bind((int64_t)id)
        .execute();

    // 更新回放的回放地址
    DBSession.sql("UPDATE live_playback SET playback_url = ? WHERE playback_id = ?")
        .bind(living_url)
        .bind(playback_id)
        .execute();

    start_ffmpeg(id, playback_id);

    // 构造返回DTO
    auto dto = StartLivingDto::createShared();
    dto->playback_id = playback_id;
    dto->living_stream_url = living_stream_url;
    dto->living_stream_code = living_stream_code;
    dto->living_url = living_url;
    result_dto->data->push_back(dto);

    result_dto->statusCode = 200;
    result_dto->message = "Start living success";
    return result_dto;
  }

  oatpp::Object<MessageDto> endLivingById(const oatpp::Int64 &id)
  {
    auto DBSession = cli.getSession();
    auto result_dto = MessageDto::createShared();

    DBSession.sql("USE xet_living_table").execute();

    // 检查直播是否是结束状态
    auto check_isliving = DBSession.sql("SELECT isliving FROM living_stream where living_stream_id=?")
                              .bind((int64_t)id)
                              .execute();
    auto check_isliving_row = *(check_isliving.begin());
    bool isliving = check_isliving_row[0].get<bool>();

    if (!isliving)
    {
      result_dto->statusCode = 500;
      result_dto->message = "Currently in ended status!";
      return result_dto;
    }

    // 清空对应字段
    DBSession.sql("UPDATE living_stream SET living_url = '' , isliving = false WHERE living_stream_id = ?")
        .bind((int64_t)id)
        .execute();

    end_ffmpeg(id);

    result_dto->statusCode = 200;
    result_dto->message = "End living success";
    return result_dto;
  }
};

#endif // Dao_HPP
