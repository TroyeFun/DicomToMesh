/****************************************************************************
** Copyright (c) 2017 Adrian Schneider, AOT AG
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
*****************************************************************************/

#include "dicomRoutines.h"

#include <vtkDICOMImageReader.h>
#include <vtkMarchingCubes.h>
#include <vtkExtractVOI.h>
#include <vtkImageThreshold.h>
#include <vtkPNGReader.h>
#include <vtkStringArray.h>
#include <iostream>
#include <vector>

using namespace std;

VTKDicomRoutines::VTKDicomRoutines()
{
    m_progressCallback = vtkSmartPointer<vtkCallbackCommand>(NULL);
}

VTKDicomRoutines::~VTKDicomRoutines()
{
}

void VTKDicomRoutines::SetProgressCallback( vtkSmartPointer<vtkCallbackCommand> progressCallback )
{
    m_progressCallback = progressCallback;
}

vtkSmartPointer<vtkImageData> VTKDicomRoutines::loadDicomImage( const std::string& pathToDicom )
{
    cout << "Read DICOM images located under " << pathToDicom << endl;

    vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName( pathToDicom.c_str() );
    if( m_progressCallback.Get() != NULL )
    {
        reader->AddObserver(vtkCommand::ProgressEvent, m_progressCallback);
    }
    reader->Update();

    vtkSmartPointer<vtkImageData> rawVolumeData = vtkSmartPointer<vtkImageData>::New();
    rawVolumeData->DeepCopy(reader->GetOutput());

    // check if load was successful
    int* dims = rawVolumeData->GetDimensions();
    if( dims[0] < 1 || dims[1] < 1 || dims[2] < 1 )
    {
        cerr << "No DICOM data in directory" << endl;
        return NULL;
    }

    cout << endl << endl;

    return rawVolumeData;
}

vtkSmartPointer<vtkPolyData> VTKDicomRoutines::dicomToMesh(vtkSmartPointer<vtkImageData> imageData, int threshold,
                                                           bool useUpperThreshold = false, int upperThreshold = 0)
{
    if(useUpperThreshold)
    {
        cout << "Create surface mesh with iso value range = " << threshold << " to " << upperThreshold << endl;

        vtkSmartPointer<vtkImageThreshold> imageThreshold = vtkSmartPointer<vtkImageThreshold>::New();
        imageThreshold->SetInputData(imageData);
        imageThreshold->ThresholdByUpper(upperThreshold);
        imageThreshold->ReplaceInOn();
        imageThreshold->SetInValue(threshold - 1); // mask voxels with a value lower than the lower threshold
        imageThreshold->Update();
        imageData->DeepCopy(imageThreshold->GetOutput());
    }
    else
    {
        cout << "Create surface mesh with iso value = " << threshold << endl;
    }

    vtkSmartPointer<vtkMarchingCubes> surfaceExtractor = vtkSmartPointer<vtkMarchingCubes>::New();
    surfaceExtractor->ComputeNormalsOn();
    surfaceExtractor->SetValue( 0,threshold ) ;
    surfaceExtractor->SetInputData( imageData );
    if( m_progressCallback.Get() != NULL )
    {
        surfaceExtractor->AddObserver(vtkCommand::ProgressEvent, m_progressCallback);
    }
    surfaceExtractor->Update();

    vtkSmartPointer<vtkPolyData> mesh = vtkSmartPointer<vtkPolyData>::New();
    mesh->DeepCopy( surfaceExtractor->GetOutput() );

    cout << endl << endl;
    return mesh;
}

void VTKDicomRoutines::cropDicom( vtkSmartPointer<vtkImageData> imageData )
{
    int* inputImgDimension = imageData->GetDimensions();
    int xd = inputImgDimension[0]; int yd = inputImgDimension[1]; int zd = inputImgDimension[2];

    // ask user for slice range
    int startSlice, endSlice, left, top, right, bottom;
    cout << "Input image sclice range from   0 - " << zd << endl;
    cout << "Start slice = ";
    int startSliceScanRes = std::scanf("%d", &startSlice);
    cout << endl << "End slice = ";
    int endSliceScanRes = std::scanf("%d", &endSlice);
    cout << endl << "Left x = (input -1 if not specified)";
    std::scanf("%d", &left);
    cout << endl << "Top y = (input -1 if not specified)";
    std::scanf("%d", &top);
    cout << endl << "Right x = (input -1 if not specified)";
    std::scanf("%d", &right);
    cout << endl << "Bottom y = (input -1 if not specified)";
    std::scanf("%d", &bottom);

    if (left == -1) left = 0;
    if (top == -1) top = 0;
    if (right == -1) right = xd-1;
    if (bottom == -1) bottom = yd-1;

    // check passed slice values
    if( startSliceScanRes != 1 || endSliceScanRes != 1 || startSlice < 0 || startSlice > endSlice || endSlice >= zd )
    {
        cout << "Invalid slice settings - skip cropping." << endl;
    }
    else
    {
        cout << "Crop from slice " << startSlice << " to " << endSlice << endl;

        vtkSmartPointer<vtkExtractVOI> cropper = vtkSmartPointer<vtkExtractVOI>::New();
        cropper->SetInputData( imageData );
        cropper->SetVOI(left, right, top, bottom, startSlice, endSlice );
        if( m_progressCallback.Get() != NULL )
        {
            cropper->AddObserver(vtkCommand::ProgressEvent, m_progressCallback);
        }
        cropper->Update();
        imageData->DeepCopy( cropper->GetOutput() );

        cout << endl << endl;
    }
}

bool VTKDicomRoutines::fileExists(const std::string& filePath)
{
    ifstream f(filePath.c_str());
        return f.good();
}

vtkSmartPointer<vtkImageData> VTKDicomRoutines::loadPngImages( const std::vector<std::string>& pngPaths,
        double x_spacing, double y_spacing, double slice_spacing )
{
    vtkSmartPointer<vtkStringArray> files = vtkSmartPointer<vtkStringArray>::New();
    files->SetNumberOfValues(pngPaths.size());

    // copy paths and check if files exist
    for( size_t i = 0; i < pngPaths.size(); i++ )
    {
        const std::string& path = pngPaths.at(i);
        if( !fileExists(path) )
        {
            cerr << "PNG file does not exist: " << path  << endl;
            return NULL;
        }

        files->SetValue(i, path);
    }

    vtkSmartPointer<vtkPNGReader> pngReader = vtkSmartPointer<vtkPNGReader>::New();
    pngReader->SetFileNames(files);
    pngReader->SetDataSpacing(x_spacing, y_spacing, slice_spacing);
    pngReader->SetDataOrigin(0, 0, 0);
    pngReader->Update();

    vtkSmartPointer<vtkImageData> rawVolumeData = vtkSmartPointer<vtkImageData>::New();
    rawVolumeData->DeepCopy(pngReader->GetOutput());

    // check if load was successful
    int* dims = rawVolumeData->GetDimensions();
    if( dims[0] < 1 || dims[1] < 1 || dims[2] < 1 )
    {
        cerr << "No PNG data in directory" << endl;
        return NULL;
    }

    cout << endl << endl;

    return rawVolumeData;
}
