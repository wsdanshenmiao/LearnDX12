#include "D3DUtil.h"

using Microsoft::WRL::ComPtr;

namespace DSM {

	// 用于创建默认缓冲区 D3D12_HEAP_TYPE_DEFAULT
	ComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		ComPtr<ID3D12Resource>& upLoadBuffer)
	{
		ComPtr<ID3D12Resource> defaultBuffer;

		// 创建默认缓冲区资源
		auto heapPropertiesDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto resourceState = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapPropertiesDefault,	// 资源堆属性
			D3D12_HEAP_FLAG_NONE,
			&resourceState,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

		// 创建一个中介的上传堆来将Cpu内存复制到默认缓冲区
		auto heapPropertiesUpLoad = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapPropertiesUpLoad,
			D3D12_HEAP_FLAG_NONE,
			&resourceState,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(upLoadBuffer.GetAddressOf())));

		// 将数据复制到默认缓冲区的步骤
		// 辅助函数UpdateSubresources会将CPU内存复制到intermediateupload堆中。
		// 然后，使用ID3D12CommandList::CopyBufferRegion或CopyTextureRegion，
		// 辅助函数的三种实现中有一种将中间上传的堆数据复制到mBuffe。
		auto commonToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->ResourceBarrier(1, &commonToCopyDest);

		// 描述要复制到默认缓冲区的资源
		D3D12_SUBRESOURCE_DATA subResourceData{};
		subResourceData.pData = initData;	// 指向包含子资源数据的内存块的指针
		subResourceData.RowPitch = byteSize;// 子资源数据的行间距
		subResourceData.SlicePitch = subResourceData.RowPitch;
		UpdateSubresources<1>(
			cmdList,
			defaultBuffer.Get(),	// 目标资源
			upLoadBuffer.Get(),		// 中介缓冲区
			0, 0, 1,
			&subResourceData);		// 源缓冲区

		auto copyDestToCommon = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ);
		cmdList->ResourceBarrier(1, &copyDestToCommon);

		// UploadBuffer不能立刻销毁，因为提交的命令列表可能尚未执行
		return defaultBuffer;
	}

	UINT D3DUtil::CalcCBByteSize(UINT byteSize) noexcept
	{
		// 常量缓冲区必须为硬件最小分配内存的整数倍，通常为256.
		// 通过给byteSize加上256后，再屏蔽掉十六进制的最后八个bit可得到256的整数倍。
		// Example: Suppose byteSize = 300.
		// (300 + 255) & ~255
		// 555 & ~255
		// 0x022B & ~0x00ff
		// 0x022B & 0xff00
		// 0x0200
		// 512
		return (byteSize + 255) & ~255;
	}

	ComPtr<ID3DBlob> D3DUtil::CompileShader(
		const WCHAR* fileName,
		const D3D_SHADER_MACRO* defines,
		const std::string& enteryPoint,
		const std::string& target,
		const WCHAR* outputFileName)
	{
		ComPtr<ID3DBlob> byteCode = nullptr;
		ComPtr<ID3DBlob> errors = nullptr;
		UINT compilFlags = D3DCOMPILE_ENABLE_STRICTNESS;	// 强制严格编译

#if defined(DEBUG) || defined(_DEBUG) 
		compilFlags |= D3DCOMPILE_DEBUG;
		compilFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;	// 跳过优化步骤
#endif

		auto hr = D3DCompileFromFile(
			fileName,
			defines,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			enteryPoint.c_str(),
			target.c_str(),
			compilFlags,
			0,
			byteCode.GetAddressOf(),
			errors.GetAddressOf());

		if (errors != nullptr) {
			OutputDebugStringA(static_cast<char*>(errors->GetBufferPointer()));
		}
		ThrowIfFailed(hr);

		if (outputFileName != nullptr) {
			ThrowIfFailed(D3DWriteBlobToFile(byteCode.Get(), outputFileName, TRUE));
		}

		return byteCode;
	}

	ComPtr<ID3DBlob> D3DUtil::LoadShaderBinary(const std::wstring& filename)
	{
		std::ifstream fin(filename, std::ios::binary);

		fin.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size = (int)fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		ComPtr<ID3DBlob> blob;
		ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

		fin.read((char*)blob->GetBufferPointer(), size);
		fin.close();

		return blob;
	}

	DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
		ErrorCode(hr),
		FunctionName(functionName),
		Filename(filename),
		LineNumber(lineNumber) {
	}


	std::wstring DxException::ToString()const
	{
		// Get the string description of the error code.
		_com_error err(ErrorCode);
		std::wstring msg = err.ErrorMessage();

		return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
	}


}
