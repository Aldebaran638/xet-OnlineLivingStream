FROM aldebaran638/ubuntu_24.04:git-cmake-oatpp_1.3.1-MySQL_9.3.0_cpp-ffmpeg-nginx_rtmp
WORKDIR /app

COPY ./xet-backend .

# RUN echo "启动后端nginx-rtmp服务器"
# WORKDIR /usr/local/nginx
# RUN ./nginx

RUN echo "启动后端异步服务器"
WORKDIR /app/xet-backend-async
RUN rm -rdf build && mkdir build
WORKDIR /app/xet-backend-async/build
RUN cmake .. && make

RUN echo "启动后端同步服务器"
WORKDIR /app/xet-backend-sync
RUN rm -rdf build && mkdir build
WORKDIR /app/xet-backend-sync/build
RUN cmake .. && make

# 执行程序
WORKDIR /app/package

RUN chmod +x run.sh 
CMD ["sh", "/app/package/run.sh"]

