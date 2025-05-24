<template>
  <div class="video-container" @mousemove="resetHideControlsTimer" @mouseleave="startHideControlsTimer">
    <video ref="videoRef" :src="videoSrc" class="video" @loadedmetadata="initVideo"
      @timeupdate="updateProgress"></video>

    <!-- 长按倍速提示 -->
    <div v-if="isSpeedUp" class="speed-up-indicator">
      <svg viewBox="0 0 24 24" width="24" height="24" class="speed-icon">
        <path fill="currentColor"
          d="M13,2.05V4.05C17.39,4.59 20.5,8.58 19.96,12.97C19.5,16.61 16.64,19.5 13,19.93V21.93C18.5,21.38 22.5,16.5 21.95,11C21.5,6.25 17.73,2.5 13,2.03V2.05M5.67,19.74C7.18,21 9.04,21.79 11,22V20C9.58,19.82 8.23,19.25 7.1,18.37L5.67,19.74M7.1,5.74C8.22,4.84 9.57,4.26 11,4.06V2.06C9.05,2.25 7.19,3.03 5.67,4.26L7.1,5.74M5.69,7.1L4.26,5.67C3,7.19 2.24,9.04 2.05,11H4.05C4.24,9.58 4.8,8.23 5.69,7.1M4.06,13H2.06C2.26,14.96 3.03,16.81 4.27,18.33L5.69,16.9C4.81,15.77 4.24,14.42 4.06,13M10,16.5V7.5L16,12L10,16.5Z" />
      </svg>
      <span>倍速播放中</span>
    </div>

    <!-- 拖动进度指示器 -->
    <div v-if="isDragging" class="drag-indicator">
      <div class="direction-indicator">
        <span v-if="dragDirection < 0" class="direction-icon">⏪</span>
        <span v-else-if="dragDirection > 0" class="direction-icon">⏩</span>
      </div>
      <div class="drag-time">{{ formatTime(currentTime) }} / {{ formatTime(duration) }}</div>
    </div>

    <!-- 拖动时的进度条指示 -->
    <div v-if="isDragging" class="drag-progress">
      <div class="drag-progress-filled" :style="{ width: `${progressPercentage}%` }"></div>
    </div>

    <!-- 添加加载中遮罩 -->
    <div v-if="isLoading || minLoadingTime" class="loading-overlay">
      <img src="../assets/loading.gif" alt="加载中..." class="loading-icon" />
    </div>

    <!-- 加载中遮罩保持不变 -->
    <div v-if="isSpeedUp" class="speed-up-indicator">
      <svg viewBox="0 0 24 24" width="24" height="24" class="speed-icon">
        <path fill="currentColor"
          d="M13,2.05V4.05C17.39,4.59 20.5,8.58 19.96,12.97C19.5,16.61 16.64,19.5 13,19.93V21.93C18.5,21.38 22.5,16.5 21.95,11C21.5,6.25 17.73,2.5 13,2.03V2.05M5.67,19.74C7.18,21 9.04,21.79 11,22V20C9.58,19.82 8.23,19.25 7.1,18.37L5.67,19.74M7.1,5.74C8.22,4.84 9.57,4.26 11,4.06V2.06C9.05,2.25 7.19,3.03 5.67,4.26L7.1,5.74M5.69,7.1L4.26,5.67C3,7.19 2.24,9.04 2.05,11H4.05C4.24,9.58 4.8,8.23 5.69,7.1M4.06,13H2.06C2.26,14.96 3.03,16.81 4.27,18.33L5.69,16.9C4.81,15.77 4.24,14.42 4.06,13M10,16.5V7.5L16,12L10,16.5Z" />
      </svg>
      <span>倍速播放中</span>
    </div>

    <!-- 弹幕层 -->
    <div class="danmu-layer">
      <div v-for="(item, index) in danmus" :key="index" class="danmu" :style="{ top: `${index * 30}px` }">
        {{ item }}
      </div>
    </div>
    <!-- 添加错误提示 -->
    <div v-if="errorState" class="error-message">
      {{ errorMessage }}
    </div>

    <!-- 添加中心播放/暂停图标 -->
    <div class="center-icon" :class="{ 'show': showCenterIcon, 'fade-out': fadeOutIcon }">
      <svg v-if="isPlaying" viewBox="0 0 24 24" width="64" height="64">
        <path fill="currentColor" d="M14,19H18V5H14M6,19H10V5H6V19Z" />
      </svg>
      <svg v-else viewBox="0 0 24 24" width="64" height="64">
        <path fill="currentColor" d="M8,5.14V19.14L19,12.14L8,5.14Z" />
      </svg>
    </div>

    <!-- 控制栏 -->
    <div class="controls" :class="{ 'hide-controls': controlsHidden }" @mouseenter="pauseHideControlsTimer"
      @mouseleave="resumeHideControlsTimer">

      <!-- 进度条容器 -->
      <div class="progress-container">
        <div class="progress-background"></div>
        <div class="progress-buffered" :style="{ width: bufferedPercentage + '%' }"></div>
        <div class="progress-fill" :style="{ width: progressPercentage + '%' }"></div>
        <input class="progress-bar" type="range" min="0" :max="duration" step="0.1" v-model="currentTime"
          @input="onSeek" />
      </div>



      <!-- 控制组容器 -->
      <div class="control-group">
        <!-- 左侧控制组 -->
        <div class="left-controls">
          <button @click.stop="togglePlay">
            <font-awesome-icon :icon="isPlaying ? 'pause' : 'play'" />
          </button>
          <button @click.stop="reload">
            <svg viewBox="0 0 24 24" width="20" height="20">
              <path fill="currentColor"
                d="M17.65,6.35C16.2,4.9 14.21,4 12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20C15.73,20 18.84,17.45 19.73,14H17.65C16.83,16.33 14.61,18 12,18A6,6 0 0,1 6,12A6,6 0 0,1 12,6C13.66,6 15.14,6.69 16.22,7.78L13,11H20V4L17.65,6.35Z" />
            </svg>
          </button>

          <!-- 音量控制 -->
          <div class="volume-control" @mouseenter="showSlider = true" @mouseleave="handleMouseLeave">
            <button @click="toggleMute">
              <svg viewBox="0 0 24 24" width="20" height="20">
                <path v-if="isMuted || volume === 0" fill="currentColor"
                  d="M12,4L9.91,6.09L12,8.18M4.27,3L3,4.27L7.73,9H3v6h4l5,5v-6.73L15.73,17l-3.09-3.09L12,13.82V18.18l3,3l1.27-1.27l0,0L4.27,3z M19.18,12c0-3.09-1.91-5.73-4.64-6.82l-1.42,1.42C15.36,7.36,17,9.5,17,12c0,2.5-1.64,4.64-3.91,5.4l1.42,1.42C17.27,17.73,19.18,15.09,19.18,12z M16.5,12c0-1.77-1-3.29-2.5-4.03V9.79L16.2,12l-2.2,2.21v1.82C15.5,15.29,16.5,13.77,16.5,12z" />
                <path v-else-if="volume < 0.5" fill="currentColor"
                  d="M5,9v6h4l5,5V4L9,9M18.5,12c0-1.77-1-3.29-2.5-4.03V16.04C17.5,15.29,18.5,13.77,18.5,12z" />
                <path v-else fill="currentColor"
                  d="M14,3.23V5.29C16.89,6.15,19,8.83,19,12C19,15.17,16.89,17.84,14,18.7V20.77C18,19.86,21,16.28,21,12C21,7.72,18,4.14,14,3.23M16.5,12C16.5,10.23,15.5,8.71,14,7.97V16C15.5,15.29,16.5,13.76,16.5,12M3,9V15H7L12,20V4L7,9H3z" />
              </svg>
            </button>

            <div class="volume-slider" v-show="showSlider" @mouseenter="hoveringSlider = true"
              @mouseleave="hoveringSlider = false">
              <div class="slider-track" @mousedown="startDrag" @click="changeVolume" ref="sliderTrack">
                <div class="slider-fill" :style="{ height: volume * 100 + '%' }"></div>
                <div class="slider-thumb" :style="{ bottom: `calc(${volume * 100}% - 7px)` }"
                  @mousedown.stop="startDrag"></div>
              </div>
            </div>
          </div>

          <!-- 添加时间显示 -->
          <div class="time-display">
            {{ formatTime(currentTime) }} / {{ formatTime(duration) }}
          </div>
        </div>

        <!-- 右侧控制组 -->
        <div class="right-controls">
          <select v-model="currentLine" @change="changeLine">
            <option v-for="(line, index) in lines" :key="index" :value="line">{{ line.name }}</option>
          </select>
          <select v-model="playbackRate" @change="changePlaybackRate">
            <option v-for="rate in rates" :key="rate" :value="rate">{{ rate }}x</option>
          </select>
          <button @click.stop="toggleFullscreen">
            <svg v-if="isFullscreen" viewBox="0 0 24 24" width="20" height="20">
              <path fill="currentColor"
                d="M5,16h3v3h2v-5H5V16z M8,8H5v2h5V5H8V8z M14,19h2v-3h3v-2h-5V19z M16,8V5h-2v5h5V8H16z" />
            </svg>
            <svg v-else viewBox="0 0 24 24" width="20" height="20">
              <path fill="currentColor"
                d="M7,14H5v5h5v-2H7v-3m-2-4h2V7h3V5H5v5m12,7h-3v2h5v-5h-2v3m2-12h-5v2h3v3h2V5z" />
            </svg>
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { inject, ref, watchEffect, computed, onMounted, onUnmounted } from 'vue'
import axios from 'axios'
import { useRoute } from 'vue-router'
import { useLiveStore } from '../store'

// 使用 store
const liveStore = useLiveStore()
const route = useRoute()
const minLoadingTime = ref(true);

// 创建计算属性，优先使用 store 中的 URL
const videoSrc = computed(() => {
  return liveStore.playback_url || './video/not_exist'
})

const lines = ref([
  { name: '线路1', url: './video/not_exist' },
  { name: '线路2', url: './video/not_exist' },
])
const currentLine = ref(lines.value[0])


const danmus = ref<string[]>([
  '【版权声明】本内容版权归提供者所有，仅限个人学习，严禁任何形式的录制、传播和账号分享。一经发现，平台将依法保留追究权，情节严重者将承担法律责任。'
])
const videoRef = ref<HTMLVideoElement>()
const isPlaying = ref(false)
const isLoading = ref(true)
const controlsHidden = ref(false)
let hideControlsTimer: number | null = null

const rates = [2, 1.5, 1, 0.5]
const playbackRate = ref(1)
// 添加错误状态相关变量
const errorState = ref(false)
const errorMessage = ref('')
const retryCount = ref(0)
const maxRetries = 3
let retryTimer: number | null = null

const isFullscreen = ref(false)
const pauseHideControls = ref(false)
const showSlider = ref(false)
const hoveringSlider = ref(false)
const volume = ref(1.0)
const isMuted = ref(false)

// 新增进度拖动相关状态
const isDraggingSlide = ref(false);
const dragStartX = ref(0);
const dragStartY = ref(0);
const dragStartTime = ref(0);
const wasPlaying = ref(false);
const gestureIntent = ref<'none' | 'drag' | 'longpress'>('none');
const dragThreshold = 10; // 水平移动超过这个距离判定为拖动意图
const dragDirection = ref(0);

// 视频播放进度状态
const currentTime = ref(0);
const duration = ref(0);

// 添加长按加速播放相关的状态
const longPressTimer = ref<number | null>(null);
const isSpeedUp = ref(false);
const originalPlaybackRate = ref(1);
const longPressThreshold = 1000;

// 添加音量相关状态
const isDragging = ref(false)
const sliderTrack = ref<HTMLElement | null>(null)
const previousVolume = ref(volume.value) // 用于保存静音前的音量

const bufferedPercentage = ref(0) // 缓冲进度条的百分比

const showCenterIcon = ref(false) // 控制中心图标的显示
const fadeOutIcon = ref(false)
let iconTimer: number | null = null

watchEffect(() => {
  if (liveStore.playback_url) {
    console.log('直播地址已更新:', liveStore.playback_url)

    // 更新所有线路为相同的直播地址
    lines.value = [
      { name: '直播线路1', url: liveStore.playback_url },
      { name: '直播线路2', url: liveStore.playback_url }
    ]

    // 默认选择第一条线路
    currentLine.value = lines.value[0]

    // 如果视频元素已经存在，直接更新源
    const video = videoRef.value
    if (video) {
      video.src = liveStore.playback_url
      video.load()

      // 无条件自动播放
      video.play()
        .then(() => {
          isPlaying.value = true;
          console.log('视频开始自动播放');
        })
        .catch(err => {
          // 如果自动播放被浏览器阻止，尝试静音播放
          if (err.name === 'NotAllowedError') {
            console.log('尝试静音播放...');
            video.muted = true;
            previousVolume.value = volume.value // 保存当前音量
            volume.value = 0
            isMuted.value = true;
            video.play()
              .then(() => {
                isPlaying.value = true;  // 添加这一行，确保静音播放时按钮状态也更新
                console.log('静音自动播放成功');
              })
              .catch(err2 => console.error('静音也无法自动播放:', err2));
          }
        });
    }
  }
})

// 计算进度条的百分比
const progressPercentage = ref(0)

// 缓冲进度条

const onSeek = () => {
  const video = videoRef.value
  if (video) {
    video.currentTime = currentTime.value
  }
}

function initVideo() {
  if (!videoRef.value) return;
  duration.value = videoRef.value.duration;
  currentTime.value = videoRef.value.currentTime;
}

// 更新进度
function updateProgress() {
  if (!videoRef.value || isDragging.value) return;
  currentTime.value = videoRef.value.currentTime;
  progressPercentage.value = (currentTime.value / duration.value) * 100 || 0;
}

// 格式化时间显示
function formatTime(seconds: number): string {
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  return `${mins}:${secs < 10 ? '0' : ''}${secs}`;
}


// 处理视频错误的函数
function handleVideoError(e: Event) {
  console.error('视频播放出错', e)
  isLoading.value = false

  // 获取详细的错误信息
  const videoElement = e.target as HTMLVideoElement
  const errorCode = videoElement?.error?.code

  // 根据错误码设置错误消息
  switch (errorCode) {
    case 1: // MEDIA_ERR_ABORTED
      errorMessage.value = '视频加载被中断 ErrNo.1'
      break
    case 2: // MEDIA_ERR_NETWORK
      errorMessage.value = '网络错误导致视频加载失败 ErrNo.2'
      break
    case 3: // MEDIA_ERR_DECODE
      errorMessage.value = '视频解码失败 ErrNo.3'
      break
    case 4: // MEDIA_ERR_SRC_NOT_SUPPORTED
      errorMessage.value = '网络错误或视频格式不受支持 ErrNo.4'
      break
    default:
      errorMessage.value = '视频加载失败，请稍后重试 ErrNo.0'
  }

  errorState.value = true

  // 自动重试逻辑
  if (retryCount.value < maxRetries) {
    // console.log(`自动重试中 (${retryCount.value + 1}/${maxRetries})...`)

    // 清除之前的计时器
    if (retryTimer) {
      clearTimeout(retryTimer)
    }

    // 设置递增的重试延迟
    const delay = 1000 * Math.pow(2, retryCount.value)
    retryTimer = window.setTimeout(() => {
      retryLoadVideo()
    }, delay)
  } else {
    errorMessage.value += '，已达到最大重试次数'
  }
}

// 重试加载视频的函数
function retryLoadVideo() {
  if (retryTimer) {
    clearTimeout(retryTimer)
    retryTimer = null
  }

  retryCount.value++
  errorState.value = false
  isLoading.value = true

  const video = videoRef.value
  if (video) {
    // 尝试重新加载当前视频
    console.log('尝试重新加载视频...')

    // 对于网络错误，可以切换线路
    if (retryCount.value >= 2 && lines.value.length > 1) {
      // 切换到下一个线路
      const currentIndex = lines.value.findIndex(line => line.url === currentLine.value.url)
      const nextIndex = (currentIndex + 1) % lines.value.length
      currentLine.value = lines.value[nextIndex]
      console.log(`切换到线路: ${currentLine.value.name}`)
    }

    // 重置视频源并重新加载
    video.src = currentLine.value.url
    video.load()

    // 尝试播放
    video.play()
      .then(() => {
        isPlaying.value = true
        console.log('重试成功，视频已开始播放')
      })
      .catch(err => {
        console.error('重试播放失败:', err)
        // 如果是自动播放策略问题，尝试静音播放
        if (err.name === 'NotAllowedError') {
          video.muted = true
          isMuted.value = true
          video.play().catch(e => console.error('静音播放也失败:', e))
        }
      })
  }
}

// 监听视频加载状态
function onBuffer() {
  console.log('正在缓冲...')
  isLoading.value = true
}

function onPlay() {
  console.log('恢复播放')
  isLoading.value = false
}

function onError(e: Event) {
  console.error('播放出错', e)
  isLoading.value = false
}

// 播放/暂停视频，并显示中心图标
function togglePlay() {
  const video = videoRef.value
  if (!video) return

  // 立即显示中心图标
  showCenterIcon.value = true
  fadeOutIcon.value = false

  // 清除现有计时器
  if (iconTimer) {
    clearTimeout(iconTimer)
    iconTimer = null
  }

  // 切换播放状态
  if (video.paused) {
    // 播放视频
    const playPromise = video.play()

    // play() 返回一个 Promise
    if (playPromise !== undefined) {
      playPromise
        .then(() => {
          // 视频成功开始播放
          isPlaying.value = true

          // 视频开始播放后再设置淡出动画
          iconTimer = window.setTimeout(() => {
            fadeOutIcon.value = true

            // 动画结束后隐藏图标
            iconTimer = window.setTimeout(() => {
              showCenterIcon.value = false
              fadeOutIcon.value = false
              iconTimer = null
            }, 500) // 淡出动画持续时间
          }, 800) // 显示持续时间
        })
        .catch(error => {
          // 视频播放失败（例如自动播放被阻止）
          console.error("播放失败:", error)
          // 恢复暂停状态
          isPlaying.value = false
          // 隐藏中心图标
          setTimeout(() => {
            fadeOutIcon.value = true
            setTimeout(() => {
              showCenterIcon.value = false
              fadeOutIcon.value = false
            }, 500)
          }, 1000)
        })
    }
  } else {
    // 暂停视频
    video.pause()
    isPlaying.value = false

  }

  resetHideControlsTimer()
}

// 触摸开始 - 初始化状态但不立即判断手势类型
function handleTouchStart(e: TouchEvent) {
  // 避免在控制区域触发手势
  if ((e.target as HTMLElement).closest('.controls')) {
    return;
  }

  // 记录初始位置
  dragStartX.value = e.touches[0].clientX;
  dragStartY.value = e.touches[0].clientY;

  const video = videoRef.value;
  if (!video) return;

  // 记录视频状态
  wasPlaying.value = !video.paused;
  dragStartTime.value = video.currentTime;

  // 初始化手势意图
  gestureIntent.value = 'none';

  // 设置长按计时器
  if (longPressTimer.value) {
    clearTimeout(longPressTimer.value);
  }

  longPressTimer.value = window.setTimeout(() => {
    // 只有在没有识别为拖动手势时才触发长按
    if (gestureIntent.value === 'none') {
      gestureIntent.value = 'longpress';

      // 激活倍速播放
      if (!isSpeedUp.value && video) {
        originalPlaybackRate.value = video.playbackRate;
        video.playbackRate = 2;
        isSpeedUp.value = true;
      }
    }
  }, longPressThreshold);
}

// 触摸移动 - 检测并确认手势类型
function handleTouchMove(e: TouchEvent) {
  const video = videoRef.value;
  if (!video) return;

  const touch = e.touches[0];
  const deltaX = touch.clientX - dragStartX.value;
  const deltaY = touch.clientY - dragStartY.value;

  // 如果已确认为长按手势，不执行拖动逻辑
  if (gestureIntent.value === 'longpress') {
    return;
  }

  // 检测是否为横向拖动意图(水平移动>垂直移动 且 超过阈值)
  if (Math.abs(deltaX) > dragThreshold && Math.abs(deltaX) > Math.abs(deltaY)) {
    // 确认为拖动手势
    if (gestureIntent.value !== 'drag') {
      gestureIntent.value = 'drag';

      // 取消长按计时器
      if (longPressTimer.value) {
        clearTimeout(longPressTimer.value);
        longPressTimer.value = null;
      }

      // 进入拖动模式
      isDraggingSlide.value = true;

      // 暂停视频以便拖动
      video.pause();
    }

    // 防止页面滚动
    e.preventDefault();

    // 执行拖动逻辑
    const containerWidth = video.clientWidth;
    const seekRatio = 0.4; // 控制灵敏度
    const timeChange = (deltaX / containerWidth) * video.duration * seekRatio;

    // 确定拖动方向
    dragDirection.value = Math.sign(deltaX);

    // 计算新时间
    let newTime = dragStartTime.value + timeChange;
    newTime = Math.max(0, Math.min(newTime, video.duration));

    // 更新视频时间
    video.currentTime = newTime;
    currentTime.value = newTime;
    progressPercentage.value = (newTime / video.duration) * 100;
  }
}

// 触摸结束 - 清理状态并完成手势
function handleTouchEnd() {
  // 清除长按计时器
  if (longPressTimer.value) {
    clearTimeout(longPressTimer.value);
    longPressTimer.value = null;
  }

  const video = videoRef.value;
  if (!video) return;

  // 处理不同手势的结束逻辑
  if (gestureIntent.value === 'drag') {
    // 如果是拖动手势结束
    isDraggingSlide.value = false;

    // 如果之前在播放，恢复播放
    if (wasPlaying.value) {
      setTimeout(() => {
        video.play().catch(() => { }); // 捕获可能的播放错误
      }, 50);
    }
  } else if (gestureIntent.value === 'longpress') {
    // 如果是长按手势结束，恢复正常速度
    if (isSpeedUp.value) {
      video.playbackRate = originalPlaybackRate.value;
      isSpeedUp.value = false;
    }
  }

  // 重置状态
  gestureIntent.value = 'none';
  dragDirection.value = 0;
}

// 触摸取消 - 与触摸结束处理相同
function handleTouchCancel() {
  handleTouchEnd();
}

function toggleMute() {
  isMuted.value = !isMuted.value

  const video = videoRef.value
  if (video) {
    video.muted = isMuted.value

    if (isMuted.value) {
      previousVolume.value = volume.value // 保存当前音量
      volume.value = 0
    } else {
      volume.value = previousVolume.value || 1
      video.volume = volume.value
    }
  }
}

// 鼠标移出音量滑块后延迟隐藏
function handleMouseLeave() {
  // 延迟消失，避免鼠标移过按钮瞬间滑块消失
  setTimeout(() => {
    if (!hoveringSlider.value) {
      showSlider.value = false
    }
  }, 1000)
}

function startDrag(event: MouseEvent) {
  event.preventDefault()
  isDragging.value = true
  updateVolumeFromMousePosition(event)

  // 添加全局事件监听器
  document.addEventListener('mousemove', dragMove)
  document.addEventListener('mouseup', stopDrag)
}

function dragMove(event: MouseEvent) {
  if (isDragging.value) {
    updateVolumeFromMousePosition(event)
  }
}

function stopDrag() {
  isDragging.value = false

  // 移除全局事件监听器
  document.removeEventListener('mousemove', dragMove)
  document.removeEventListener('mouseup', stopDrag)
}

function updateVolumeFromMousePosition(event: MouseEvent) {
  if (!sliderTrack.value) return

  const track = sliderTrack.value.getBoundingClientRect()
  const y = event.clientY

  // 只有在鼠标仍在 track 高度附近时更新音量
  if (y < track.top - 50 || y > track.bottom + 50) return

  const percent = 1 - Math.max(0, Math.min(1, (y - track.top) / track.height))
  volume.value = percent

  const video = videoRef.value
  if (video) {
    video.volume = volume.value
    if (volume.value > 0) {
      video.muted = false
      isMuted.value = false
    }
  }
}

function changeVolume(event: MouseEvent) {
  updateVolumeFromMousePosition(event)
}

function reload() {
  const video = videoRef.value
  if (!video) return

  // 保存当前播放位置和播放状态
  const currentPosition = video.currentTime
  const wasPlaying = !video.paused

  // 重新加载视频
  video.load()

  // 视频加载后设置到原来的播放位置
  video.onloadeddata = () => {
    video.currentTime = currentPosition
    if (wasPlaying) {
      video.play()
      isPlaying.value = true
    }
  }
  resetHideControlsTimer()
}

function changeLine() {
  const video = videoRef.value
  if (!video) return

  // 保存当前播放位置和播放状态
  const currentPosition = video.currentTime
  const wasPlaying = !video.paused

  // 直接设置视频元素的 src，而不是修改计算属性
  video.src = currentLine.value.url

  // 加载新视频源
  video.load()

  // 等待视频加载完成后设置播放位置并播放
  video.onloadeddata = () => {
    // 设置回之前的播放位置
    video.currentTime = currentPosition

    // 如果之前是播放状态，则继续播放
    if (wasPlaying) {
      video.play()
        .catch(err => console.error('无法自动播放:', err))
      isPlaying.value = true
    }
  }

  resetHideControlsTimer()
}

function changePlaybackRate() {
  const video = videoRef.value
  if (video) video.playbackRate = playbackRate.value
  resetHideControlsTimer()
}

function toggleFullscreen() {
  const container = document.querySelector('.video-container')
  if (!container) return

  if (!document.fullscreenElement) {
    // 进入全屏模式
    if (container.requestFullscreen) {
      container.requestFullscreen()
      isFullscreen.value = true
    }
  } else {
    // 退出全屏模式
    if (document.exitFullscreen) {
      document.exitFullscreen()
      isFullscreen.value = false
    }
  }
  resetHideControlsTimer()
}

onMounted(() => {

  const videoContainer = document.querySelector('.video-container');
  if (videoContainer) {
    videoContainer.addEventListener('touchstart', handleTouchStart as EventListener, { passive: true });
    videoContainer.addEventListener('touchmove', handleTouchMove as EventListener, { passive: false }); // 允许阻止默认行为
    videoContainer.addEventListener('touchend', handleTouchEnd);
    videoContainer.addEventListener('touchcancel', handleTouchCancel);
  }

  const video = videoRef.value
  setTimeout(() => {
    minLoadingTime.value = false;
  }, 1000);
  if (!video) return

  if (video) {
    video.addEventListener('stalled', onBuffer) // 网络不佳也可触发
    video.addEventListener('canplay', onPlay)
  }

  const updateBuffered = () => {
    if (video.buffered.length) {
      const end = video.buffered.end(video.buffered.length - 1)
      bufferedPercentage.value = (end / video.duration) * 100
    }
  }

  const updateProgress = () => {
    currentTime.value = video.currentTime
  }

  const updateDuration = () => {
    duration.value = video.duration
  }

  video.addEventListener('progress', updateBuffered)
  video.addEventListener('timeupdate', updateProgress)
  video.addEventListener('loadedmetadata', updateDuration)

  // 清除监听器（推荐）
  onUnmounted(() => {
    video.removeEventListener('progress', updateBuffered)
    video.removeEventListener('timeupdate', updateProgress)
    video.removeEventListener('stalled', onBuffer)
    video.removeEventListener('canplay', onPlay)
    video.removeEventListener('loadedmetadata', updateDuration)
  })
})


function handleFullscreenChange() {
  isFullscreen.value = !!document.fullscreenElement
}

// 控制栏自动隐藏相关函数
function startHideControlsTimer() {
  if (hideControlsTimer) clearTimeout(hideControlsTimer)

  // 仅当不在暂停状态时才启动隐藏计时器
  if (!pauseHideControls.value) {
    hideControlsTimer = window.setTimeout(() => {
      controlsHidden.value = true
    }, 3000) // 3秒后隐藏
  }
}

function resetHideControlsTimer() {
  controlsHidden.value = false
  if (hideControlsTimer) clearTimeout(hideControlsTimer)

  // 仅当不在暂停状态时才启动隐藏计时器
  if (!pauseHideControls.value) {
    startHideControlsTimer()
  }
}

function pauseHideControlsTimer() {
  pauseHideControls.value = true
  if (hideControlsTimer) {
    clearTimeout(hideControlsTimer)
    hideControlsTimer = null
  }
  // 确保控制栏可见
  controlsHidden.value = false
}

function resumeHideControlsTimer() {
  pauseHideControls.value = false
  // 恢复自动隐藏逻辑
  startHideControlsTimer()
}



onUnmounted(() => {
  // 移除触摸事件监听
  const videoContainer = document.querySelector('.video-container');
  if (videoContainer) {
    videoContainer.removeEventListener('touchstart', handleTouchStart as EventListener);
    videoContainer.removeEventListener('touchmove', handleTouchMove as EventListener);
    videoContainer.removeEventListener('touchend', handleTouchEnd);
    videoContainer.removeEventListener('touchcancel', handleTouchCancel);
  }

  // 清除长按计时器
  if (longPressTimer.value) {
    clearTimeout(longPressTimer.value);
    longPressTimer.value = null;
  }

  document.removeEventListener('fullscreenchange', handleFullscreenChange)
  document.removeEventListener('mousemove', dragMove)
  document.removeEventListener('mouseup', stopDrag)
  if (hideControlsTimer) clearTimeout(hideControlsTimer)
  if (iconTimer) clearTimeout(iconTimer)
  if (retryTimer) {
    clearTimeout(retryTimer)
    retryTimer = null
  }
})

// 初始化时启动自动隐藏定时器
startHideControlsTimer()
</script>

<style scoped>
.video-container {
  position: relative;
  width: 100%;
  height: 100%;
  max-height: 100vh;
  background: black;
  display: flex;
  justify-content: center;
  align-items: center;
  overflow: hidden;
}

video {
  width: 100%;
  height: 100%;
  object-fit: contain;
  max-height: 100%;
}

.video-wrapper {
  position: relative;
}

.danmu-layer {
  position: absolute;
  top: 10%;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
  /* 不干扰点击 */
  overflow: hidden;
  z-index: 5;
}

.danmu {
  position: absolute;
  white-space: nowrap;
  font-size: 18px;
  color: white;
  text-shadow: 1px 1px 2px black;
  animation: danmu-move 30s linear infinite;
}

@keyframes danmu-move {
  0% {
    transform: translateX(150%);
  }

  100% {
    transform: translateX(-100%);
  }
}

.loading-overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(255, 255, 255, 0);
  /* 可选半透明背景 */
  display: flex;
  align-items: center;
  outline: none;
  justify-content: center;
  z-index: 0;
}

.loading-icon {
  width: 48px;
  height: 48px;
  animation: spin 1s linear infinite;
}

.volume-control {
  position: relative;
  display: inline-block;
}

.volume-slider {
  position: absolute;
  bottom: 30px;
  left: 50%;
  transform: translateX(-50%);
  height: 100px;
  width: 30px;
  background: #00000091;
  border-radius: 20px;
  display: flex;
  align-items: flex-end;
  padding: 12px;
  box-shadow: 0 0 4px rgba(0, 0, 0, 0.2);
  z-index: 20;
}

.slider-thumb {
  position: absolute;
  left: 50%;
  transform: translateX(-50%);
  width: 14px;
  height: 14px;
  background-color: white;
  border-radius: 50%;
  cursor: pointer;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
  transition: transform 0.1s ease;
  z-index: 3;
}

.slider-thumb:hover,
.slider-thumb:active {
  transform: translateX(-50%) scale(1.1);
}

.slider-track {
  position: relative;
  width: 100%;
  height: 100%;
  background: #ccc;
  border-radius: 4px;
  cursor: pointer;
}

.slider-fill {
  position: absolute;
  bottom: 0;
  width: 100%;
  background: #007bff;
  border-radius: 4px;
}

/* 确保鼠标拖拽时不会选中文本 */
.volume-slider {
  user-select: none;
}

/* 中心图标 */
.center-icon {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  background-color: rgba(0, 0, 0, 0.6);
  border-radius: 50%;
  width: 80px;
  height: 80px;
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  opacity: 0;
  pointer-events: none;
  /* 不要拦截点击事件 */
  transition: opacity 0.3s ease-in-out;
}

.center-icon.show {
  opacity: 0.8;
}

.center-icon.fade-out {
  opacity: 0;
}

.center-icon svg {
  filter: drop-shadow(0 0 3px rgba(0, 0, 0, 0.5));
}

/* 控制栏 */
.controls {
  position: absolute;
  bottom: 0px;
  left: 0px;
  right: 0px;
  display: flex;
  flex-direction: column;
  gap: 6px;
  background-color: rgba(0, 0, 0, 0.7);
  border-radius: 4px;
  color: white;
  padding: 8px;
  transition: transform 0.5s ease, opacity 0.5s ease;
}

.hide-controls {
  transform: translateY(100%);
  opacity: 0;
}

.control-group {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.left-controls,
.right-controls {
  display: flex;
  gap: 10px;
  align-items: center;
}

.error-message {
  position: fixed;
  top: 20px;
  left: 50%;
  transform: translateX(-50%);
  background-color: rgba(255, 0, 0, 0.7);
  color: white;
  padding: 10px 20px;
  border-radius: 4px;
  z-index: 1000;
  animation: fadeIn 0.5s ease, fadeOut 0.5s ease 4.5s;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }

  to {
    opacity: 1;
  }
}

@keyframes fadeOut {
  from {
    opacity: 1;
  }

  to {
    opacity: 0;
  }
}

.progress-container {
  position: relative;
  width: 100%;
  height: 15px;
  display: flex;
  align-items: center;
}

.progress-background {
  position: absolute;
  height: 3px;
  width: 100%;
  background: rgba(255, 255, 255, 0.3);
  border-radius: 2px;
}

.progress-buffered {
  position: absolute;
  height: 3px;
  background: rgba(255, 255, 255, 0.6);
  /* 缓冲条是浅色 */
  border-radius: 2px;
  z-index: 0;
  /* 在背景条之上，播放条之下 */
}

.progress-fill {
  position: absolute;
  height: 3px;
  background: #1976D2;
  /* 蓝色 */
  border-radius: 2px;
  z-index: 1;
}

.progress-bar {
  position: relative;
  width: 100%;
  height: 15px;
  -webkit-appearance: none;
  appearance: none;
  background: transparent;
  outline: none;
  z-index: 2;
}

.progress-bar::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: white;
  cursor: pointer;
}

.progress-bar::-moz-range-thumb {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: white;
  cursor: pointer;
}

.time-display {
  font-family: monospace;
  font: 1em sans-serif;
  /* 等宽字体确保数字对齐 */
  font-size: 14px;
  color: white;
  margin: 0 10px;
  min-width: 85px;
  /* 确保宽度稳定，避免UI跳动 */
  text-align: center;
}

button {
  background: none;
  border: none;
  color: white;
  font-size: 16px;
  width: 30px;
  height: 30px;
  border-radius: 50%;
  display: flex;
  justify-content: center;
  align-items: center;
  transition: background-color 0.3s;
}

button:hover {
  background-color: rgba(51, 51, 51, 0.452);
}

select {
  background-color: rgba(0, 0, 0, 0.5);
  color: white;
  border: 1px solid rgba(255, 255, 255, 0.3);
  border-radius: 3px;
  padding: 3px 5px;
  font-size: 14px;
}

.drag-indicator {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  background-color: rgba(0, 0, 0, 0.7);
  color: white;
  padding: 12px 20px;
  border-radius: 20px;
  display: flex;
  flex-direction: column;
  align-items: center;
  z-index: 30;
}

.direction-indicator {
  font-size: 22px;
  margin-bottom: 5px;
}

.drag-time {
  font-size: 16px;
  font-weight: 600;
}

/* 进度条指示样式 */
.drag-progress {
  position: absolute;
  bottom: 60px;
  left: 10%;
  width: 80%;
  height: 4px;
  background-color: rgba(255, 255, 255, 0.3);
  border-radius: 2px;
  z-index: 30;
}

.drag-progress-filled {
  height: 100%;
  background-color: #ff3d00;
  border-radius: 2px;
  transition: width 0.05s linear;
}

/* 倍速指示器样式 */
.speed-up-indicator {
  position: absolute;
  top: 20px;
  left: 50%;
  transform: translateX(-50%);
  background-color: rgba(0, 0, 0, 0.7);
  color: white;
  padding: 8px 16px;
  border-radius: 20px;
  display: flex;
  align-items: center;
  gap: 8px;
  z-index: 30;
}

.speed-icon {
  color: #ff9800;
}
</style>