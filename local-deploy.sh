#!/bin/bash

# local-deploy.sh
# 本地模拟 GitHub Actions 的 CI/CD 构建流程
# 使用场景：内网 WSL 无法从 GitHub Actions 自动拉取代码部署

set -e  # 出错即退出
PROJECT_DIR="/home/"

echo "1. 拉取/更新 GitHub dev 分支代码..."
cd "$PROJECT_DIR"
    git clone -b dev https://github.com/Aldebaran638/xet-OnlineLivingStream.git "$PROJECT_DIR"


echo "-------------------------------------"
echo "2. 进入部署目录执行初始化"
cd "$PROJECT_DIR/xet-OnlineLivingStream/xet-operation/"
sh ./deploy-init.sh

echo "-------------------------------------"
echo "3. 构建前端镜像（自动构建 Vite 项目）..."
cd "$PROJECT_DIR/xet-OnlineLivingStream"
docker build -f ./xet-operation/frontend/Dockerfile -t xet-frontend-img .

echo "-------------------------------------"
echo "4. 构建后端镜像..."
cd "$PROJECT_DIR/xet-OnlineLivingStream"
docker build -f ./xet-operation/backend/Dockerfile -t xet-backend-img .

echo "5. 启动 docker compose 服务"
cd "$PROJECT_DIR/xet-OnlineLivingStream"
docker-compose -f './xet-operation/docker-compose.yml' up -d --build

echo "部署完成！请访问：https://lcmonitor.dynv6.net"
