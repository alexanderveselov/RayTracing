#include "cl_context.hpp"
#include <iostream>
#include <vector>
#include <string>

// If we got an error, how to terminate the construction of the object?
ClContext::ClContext(const cl::Platform& platform, const std::string& source, size_t width, size_t height, size_t cell_resolution) : width_(width), height_(height), valid_(true)
{
    std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
    std::vector<cl::Device> platform_devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);

    if (platform_devices.size() == 0)
    {
        std::cerr << "No devices found!" << std::endl;
        valid_ = false;
    }

    for (int i = 0; i < platform_devices.size(); ++i)
    {
        std::cout << "Device: " << std::endl;
        std::cout << platform_devices[i].getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Status: " << (platform_devices[i].getInfo<CL_DEVICE_AVAILABLE>() ? "Available" : "Not available") << std::endl;
        std::cout << "Max compute units: " << platform_devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        std::cout << "Max workgroup size: " << platform_devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
    }
    std::cout << std::endl;

    cl_int errCode;
    context_ = cl::Context(platform_devices, 0, 0, 0, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create context! (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    
    cl::Program program(context_, source);

    errCode = program.build(platform_devices);
    if (errCode != CL_SUCCESS)
    {
        std::cerr << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << " (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(platform_devices[0]) << std::endl;

    kernel_ = cl::Kernel(program, "main", &errCode);
    if (errCode)
    {
	    std::cerr << "Cannot create kernel! (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    
    queue_ = cl::CommandQueue(context_, platform_devices[0], 0, &errCode);
    if (errCode)
    {
	    std::cerr << "Cannot create queue! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

}

void ClContext::setupBuffers(const Scene& scene)
{
    // Memory leak!
    size_t global_work_size = width_ * height_;
    int* random_array = new int[global_work_size];

    for (size_t i = 0; i < width_ * height_; ++i)
    {
        random_array[i] = rand();
    }

    cl_int errCode;
    pixel_buffer_  = cl::Buffer(context_, CL_MEM_READ_WRITE, global_work_size * sizeof(cl_float4), 0, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create pixel buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    random_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, global_work_size * sizeof(cl_int), random_array, &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create random buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    scene_buffer_  = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.triangles.size() * sizeof(Triangle), (void*) scene.triangles.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create scene buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    index_buffer_  = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.indices.size() * sizeof(cl_uint), (void*) scene.indices.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create index buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }

    cell_buffer_   = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.cells.size() * sizeof(CellData), (void*) scene.cells.data(), &errCode);
    if (errCode)
    {
        std::cerr << "Cannot create cell buffer! (" << errCode << ")" << std::endl;
        valid_ = false;
    }
    
	setArgument(0, sizeof(cl::Buffer), &pixel_buffer_);
	setArgument(1, sizeof(cl::Buffer), &random_buffer_);
	
	setArgument(2, sizeof(size_t), &width_);
	setArgument(3, sizeof(size_t), &height_);
	
	setArgument(7, sizeof(cl::Buffer), &scene_buffer_);
	setArgument(8, sizeof(cl::Buffer), &index_buffer_);
	setArgument(9, sizeof(cl::Buffer), &cell_buffer_);
    
    //writeBuffer(random_buffer_, global_work_size * sizeof(int), random_array);

}

void ClContext::setArgument(size_t index, size_t size, const void* argPtr)
{
    kernel_.setArg(index, size, argPtr);
}

void ClContext::writeBuffer(const cl::Buffer& buffer, size_t size, const void* ptr)
{
    queue_.enqueueWriteBuffer(buffer, true, 0, size, ptr);
	//queue_.finish();

}

void ClContext::executeKernel()
{
    queue_.enqueueNDRangeKernel(kernel_, cl::NullRange, cl::NDRange(width_ * height_));
	queue_.finish();

}

cl_float4* ClContext::getPixels()
{
    cl_float4* ptr = static_cast<cl_float4*>(queue_.enqueueMapBuffer(pixel_buffer_, CL_TRUE, CL_MAP_READ, 0, width_ * height_ * sizeof(cl_float4)));
    //queue_.finish();
    return ptr;

}

void ClContext::unmapPixels(cl_float4* ptr)
{
    queue_.enqueueUnmapMemObject(pixel_buffer_, ptr);
	//queue_.finish();

}