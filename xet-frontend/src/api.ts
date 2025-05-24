import axios, { AxiosError } from 'axios'
import type { AxiosRequestConfig, AxiosResponse } from 'axios'

// 配置 axios 默认设置
const apiClient = axios.create({
    baseURL: import.meta.env.VITE_API_BASE_URL || 'http://lcmonitor.dynv6.net:8001', // 从环境变量获取基础URL
    timeout: 10000, // 请求超时时间
    headers: {
        'Content-Type': 'application/json'
    }
})

// 请求拦截器：添加认证信息、日志记录等
apiClient.interceptors.request.use(
    (config) => {
        // 获取存储在本地的 token
        const token = localStorage.getItem('auth_token')
        if (token) {
            config.headers['Authorization'] = `Bearer ${token}`
        }

        // 记录请求信息（开发环境）
        if (import.meta.env.DEV) {
            console.log(`API 请求: ${config.method?.toUpperCase()} ${config.baseURL}${config.url}`)
        }

        return config
    },
    (error) => {
        return Promise.reject(error)
    }
)

// 响应拦截器：统一错误处理、数据转换等
apiClient.interceptors.response.use(
    (response) => {
        // 可以在这里处理通用的响应逻辑
        return response
    },
    (error: AxiosError) => {
        // 统一错误处理
        if (error.response) {
            // 服务器返回了错误状态码
            const status = error.response.status

            switch (status) {
                case 401:
                    // 未授权，可能需要重新登录
                    console.error('未授权访问，请重新登录')
                    // 可以触发重定向到登录页面
                    // router.push('/login')
                    break
                case 403:
                    console.error('无权限访问此资源')
                    break
                case 404:
                    console.error('请求的资源不存在')
                    break
                case 500:
                    console.error('服务器内部错误')
                    break
                default:
                    console.error(`未处理的错误状态码: ${status}`)
            }
        } else if (error.request) {
            // 请求已经发出，但没有收到响应
            console.error('网络错误，无法连接到服务器')
        } else {
            // 请求配置出错
            console.error('请求配置错误:', error.message)
        }

        return Promise.reject(error)
    }
)

// API 响应通用接口
export interface ApiResponse<T> {
    statusCode: number;
    message: string;
    data: T[];
}

// 直播信息接口
export interface LiveStreamInfo {
    living_stream_id: number;
    creator_user_id: number;
    description: string;
    playback_url: string;
    page_count_comment: number;
    page_count_explanation: number;
    page_count_file: number;
    comments: Comment[];
    explanations: Explanation[];
    files: File[];
}

// 评论接口
export interface Comment {
    comment_id: number;
    living_stream_id: number;
    creator_user_id: number;
    created_at: string;
    content: string;
    sending?: boolean; // 可选属性，表示评论是否正在发送
    error?: boolean; // 可选属性，表示评论发送是否失败
}

// 讲解接口
export interface Explanation {
    expla_id: number;
    living_stream_id: number;
    creator_user_id: number;
    created_at: string;
    content: string;
    sending?: boolean;
    error?: boolean;
}

// 文件接口
export interface File {
    file_id: number;
    living_stream_id: number;
    creator_user_id: number;
    created_at: string;
    file_url: string;
    file_size?: number; // 可选属性，文件大小
    sending?: boolean; 
    error?: boolean;
}

// API 方法
export const liveAPI = {
    // 获取直播信息（包含评论、讲解、文件）
    fetchLiveInfo: async (liveId: number | string): Promise<ApiResponse<LiveStreamInfo>> => {
        try {
            const response = await apiClient.get(`/home/live/${liveId}`);
            console.log('获取到直播信息:', response.data);
            return response.data;
        } catch (error) {
            console.error('获取直播信息失败:', error);
            throw error;
        }
    },

    // 分页获取评论
    fetchCommentsByPage: async (liveId: number | string, page: number): Promise<ApiResponse<Comment>> => {
        try {
            const response = await apiClient.get(`/home/live/${liveId}/comment/${page}`);
            return response.data;
        } catch (error) {
            console.error('获取评论失败:', error);
            throw error;
        }
    },

    // 分页获取讲解
    fetchExplanationsByPage: async (liveId: number | string, page: number): Promise<ApiResponse<Explanation>> => {
        try {
            const response = await apiClient.get(`/home/live/${liveId}/expla/${page}`);
            return response.data;
        } catch (error) {
            console.error('获取讲解失败:', error);
            throw error;
        }
    },

    // 分页获取文件
    fetchFilesByPage: async (liveId: number | string, page: number): Promise<ApiResponse<File>> => {
        try {
            const response = await apiClient.get(`/home/live/${liveId}/file/${page}`);
            return response.data;
        } catch (error) {
            console.error('获取文件失败:', error);
            throw error;
        }
    },

    // 添加评论
    addComment: async (liveId: number | string, content: string, userId: number): Promise<ApiResponse<any>> => {
        try {
            const data = {
                living_stream_id: liveId,
                creator_user_id: userId,
                content: content,
                created_at: new Date(new Date().getTime() + 8 * 60 * 60 * 1000).toISOString().replace('T', ' ').slice(0, 19),
            };
            const response = await apiClient.post(`/home/live/${liveId}/comment`, data);
            return response.data;
        } catch (error) {
            console.error('获取评论失败:', error);
            throw error;
        }
    },

    // 添加讲解
    addExplanation: async (liveId: number | string, content: string, userId: number): Promise<ApiResponse<any>> => {
        try {
            const data = {
                living_stream_id: liveId,
                creator_user_id: userId,
                content: content,
                created_at: new Date().toISOString().replace('T', ' ').slice(0, 19),
            };
            const response = await apiClient.post(`/home/live/${liveId}/expla`, data);
            return response.data;
        } catch (error) {
            console.error('获取讲解失败:', error);
            throw error;
        }
    },

    // 添加文件
    addFile: async (liveId: number | string, fileUrl: string, userId: number): Promise<ApiResponse<any>> => {
        try {
            const data = {
                living_stream_id: liveId,
                creator_user_id: userId,
                file_url: fileUrl,
                created_at: new Date().toISOString().replace('T', ' ').slice(0, 19),
            };
            const response = await apiClient.post(`/home/live/${liveId}/file`, data);
            return response.data;
        } catch (error) {
            console.error('获取文件失败:', error);
            throw error;
        }
    }
};

// 导出默认配置的 axios 实例
export default apiClient

