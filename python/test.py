import os
import sys
import numpy as np
import imageio
import pycuda.driver as cuda
import pycuda.autoinit
import tensorrt as trt
import cv2

import numpy as np
from skimage.transform import resize

ENGINE_PATH = '/mnt/hdd2/chenbeitao/code/TensorRT/model/YOLOX/YOLOX_outputs/yolox_s/model_trt.engine'
CROP_SIZE = (640, 640) # ADJUST
INPUT_H = 640
INPUT_W = 640
NUM_CLASS = 80
INPUT_DATA_TYPE = np.float32 # ADJUST
MEASURE_TIME = True # ADJUST
CLASS_THRESHOLD = 0.3
NMS_THRESHOLD = 0.45
CALC_VAL_ACCURACY = True # ADJUST

def prepare_image(img_in, crop_size):
    img = resize_and_crop(img_in, crop_size)
    img = img.astype(INPUT_DATA_TYPE)
    img = img.transpose(2, 0, 1) # to CHW
    return img

def read_img():
    # img = imageio.imread(sys.argv[1], pilmode='RGB')
    # img = prepare_image(img, CROP_SIZE)
    img = cv2.imread(sys.argv[1])
    scale = min(INPUT_W / img.shape[1], INPUT_H / img.shape[0])
    img_w, img_h = img.shape[1], img.shape[0]
    r = min(INPUT_W / img.shape[1], INPUT_H / img.shape[0])
    unpad_w = int(r * img.shape[1])
    unpad_h = int(r * img.shape[0])
    img = cv2.resize(img, (unpad_w, unpad_h))
    out = np.ones((INPUT_H, INPUT_W, 3)) * 114
    out[:img.shape[0], :img.shape[1], :] = img
    img = out.transpose(2, 0, 1)
    
    return img, scale, img_w, img_h

def resize_and_crop(img_in, crop_size):
    '''
    Resize input image to fit the crop_size, keeping the aspect ratio
    Then crop
    -> img_in: input image
    -> crop_size: crop size
    <- resized and cropped image
    '''
    img = img_in.copy()

    if crop_size[1]/crop_size[0] < img.shape[1]/img.shape[0]:
        new_h = crop_size[0]
        new_w = int(np.ceil(crop_size[0] * img.shape[1] / img.shape[0]))
    else:
        new_h = int(np.ceil(crop_size[1] * img.shape[0] / img.shape[1]))
        new_w = crop_size[1]

    img = resize(img, (new_h, new_w), order=3, mode='reflect', anti_aliasing=True, preserve_range=True)

    offset = ((img.shape[0] - crop_size[0]) // 2,
              (img.shape[1] - crop_size[1]) // 2)
    img = img[offset[0]:offset[0]+crop_size[0],
              offset[1]:offset[1]+crop_size[1], :]

    return img

def generate_grids_and_stride(strides):
    output_grid = []
    for stride in strides:
        num_grid_y = INPUT_H // stride
        num_grid_x = INPUT_W // stride
        grid_y = np.linspace(0, num_grid_y - 1, num_grid_y)
        grid_x = np.linspace(0, num_grid_x - 1, num_grid_x)
        grid_x, grid_y = np.meshgrid(grid_x, grid_y)
        grid_corr = np.concatenate([grid_x.reshape(-1, 1), grid_y.reshape(-1, 1)], axis=1)
        temp_grid = np.concatenate([grid_corr, np.ones((grid_corr.shape[0], 1)) * stride], axis=1)
        output_grid.append(temp_grid)
        
    output_grid = np.concatenate(output_grid, axis=0)
    
    return output_grid

def generate_yolo_proposals(host_out, grid_strides):
    '''
    # print(host_out.shape)     (8400, 85)  [x0, y0, w, h, score, ...80...]
    # print(grid_strides.shape) (8400, 3)   [x, y, stride]
    '''
    output = []
    x_center = (host_out[:, 0] + grid_strides[:, 0]) * grid_strides[:, 2]
    y_center = (host_out[:, 1] + grid_strides[:, 1]) * grid_strides[:, 2]
    w = np.exp(host_out[:, 2]) * grid_strides[:, 2]
    h = np.exp(host_out[:, 3]) * grid_strides[:, 2]
    x0 = x_center - w * 0.5
    y0 = y_center - h * 0.5
    objectness = host_out[:, 4].reshape(-1, 1)
    host_out[:, 5:] *= objectness
    
    for class_idx in range(NUM_CLASS):
        choose_id = host_out[:, 5 + class_idx] > CLASS_THRESHOLD
        # print(x0[choose_id].shape)
        if choose_id.sum() == 0:
            continue
        output.append(
            np.concatenate([
                x0[choose_id].reshape(-1, 1), 
                y0[choose_id].reshape(-1, 1),
                w[choose_id].reshape(-1, 1),
                h[choose_id].reshape(-1, 1),
                np.ones((x0[choose_id].shape[0], 1), dtype=np.int32) * class_idx,
                host_out[choose_id, 5 + class_idx].reshape(-1, 1)
            ], axis=1)
        )
    output = np.concatenate(output, axis=0)
    
    return output

def intersection_area(temp_a, temp_b):
    xa0, ya0, wa, ha = temp_a[0], temp_a[1], temp_a[2], temp_a[3]
    xb0, yb0, wb, hb = temp_b[0], temp_b[1], temp_b[2], temp_b[3]
    xa1, ya1 = xa0 + wa, ya0 + ha
    xb1, yb1 = xb0 + wb, yb0 + hb
    x0 = max(xa0, xb0)
    y0 = max(ya0, yb0)
    x1 = min(xa1, xb1)
    y1 = min(ya1, yb1)
    w = max(0, x1 - x0)
    h = max(0, y1 - y0)
    
    return w * h
    
    
def nms_sorted_bboxes(proposals):
    picked = []
    results = []
    num_proposals = proposals.shape[0]
    for i in range(num_proposals):
        temp_a = proposals[i]
        keep = 1
        for j in picked:
            temp_b = proposals[j]
            inter_area = intersection_area(temp_a, temp_b)
            print(inter_area)
            union_area = temp_a[2] * temp_a[3] + temp_b[2] * temp_b[3] - inter_area
            print(inter_area / union_area)
            if inter_area / union_area > NMS_THRESHOLD:
                # print(temp_a)
                # print(temp_b)
                # print(inter_area / union_area)
                keep = 0
                break
        if keep:
            picked.append(i)
            results.append(proposals[i])
    # print(results)
    return results

def draw_box(proposals):
    img = cv2.imread(sys.argv[1])
    for proposal in proposals:
        x0, y0, w, h = int(proposal[0]), int(proposal[1]), int(proposal[2]), int(proposal[3])
        cv2.rectangle(img, (x0, y0), (x0+w, y0+h), (0, 255, 0), 3)
    cv2.imwrite('./test.jpg', img)

def decode_outputs(host_out, scale, img_w, img_h):
    strides = [8, 16, 32]
    grid_strides = generate_grids_and_stride(strides)
    host_out = host_out.reshape(-1, NUM_CLASS + 5)
    proposals = generate_yolo_proposals(host_out, grid_strides)
    idx = np.argsort(proposals[:, -1])[::-1]
    proposals = proposals[idx]
    proposals = nms_sorted_bboxes(proposals)
    print(len(proposals))
    for proposal in proposals:
        x0 = proposal[0] / scale
        y0 = proposal[1] / scale
        x1 = (proposal[0] + proposal[2]) / scale
        y1 = (proposal[1] + proposal[3]) / scale
        
        x0 = np.clip(x0, 0, img_w)
        y0 = np.clip(y0, 0, img_h)
        x1 = np.clip(x1, 0, img_w)
        y1 = np.clip(y1, 0, img_h)
        
        proposal[0], proposal[1], proposal[2], proposal[3] = x0, y0, x1 - x0, y1 - y0
        
    draw_box(proposals)

def inference():
    
    trt_logger = trt.Logger(trt.Logger.INFO)
    runtime = trt.Runtime(trt_logger)
    with open(ENGINE_PATH, 'rb') as f:
        engine = runtime.deserialize_cuda_engine(f.read())
        context = engine.create_execution_context()
        
    INPUT_DATA_TYPE = np.float32
    stream = cuda.Stream()
    host_in = cuda.pagelocked_empty(trt.volume(engine.get_binding_shape(0)), dtype=INPUT_DATA_TYPE)
    host_out = cuda.pagelocked_empty(trt.volume(engine.get_binding_shape(1)), dtype=INPUT_DATA_TYPE)
    devide_in = cuda.mem_alloc(host_in.nbytes)
    devide_out = cuda.mem_alloc(host_out.nbytes)
    
    
    img, scale, img_w, img_h = read_img()
    bindings = [int(devide_in), int(devide_out)]
    np.copyto(host_in, img.ravel())
    cuda.memcpy_htod_async(devide_in, host_in, stream)
    context.execute_async(bindings=bindings, stream_handle=stream.handle)
    cuda.memcpy_dtoh_async(host_out, devide_out, stream)
    stream.synchronize()
    
    # print(type(host_out))
    # print(host_out.shape)
    # print(len(host_out))
    
    decode_outputs(host_out, scale, img_w, img_h)
    

if __name__ == '__main__':
    inference()
    