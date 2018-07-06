/*
 * Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "gstCamera.h"

#include "glDisplay.h"
#include "glTexture.h"

#include <stdio.h>
#include <stdlib.h> 		// Contiene a system
#include <unistd.h>		// Contiene sleep()
#include <signal.h>
#include <unistd.h>

#include "cudaNormalize.h"
#include "cudaFont.h"
#include "imageNet.h"


#define DEFAULT_CAMERA -1	// -1 for onboard camera, or change to index of /dev/video V4L2 camera (>=0)	
		
		
		
bool signal_recieved = false;

void sig_handler(int signo)
{
	if( signo == SIGINT )
	{
		printf("received SIGINT\n");
		signal_recieved = true;
	}
}


int main( int argc, char** argv )
{
	printf("imagenet-camera\n  args (%i):  ", argc);

	for( int i=0; i < argc; i++ )
		printf("%i [%s]  ", i, argv[i]);
		
	printf("\n\n");
	

	/*
	 * attach signal handler
	 */
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		printf("\ncan't catch SIGINT\n");


	/*
	 * create the camera device
	 */
	gstCamera* camera = gstCamera::Create(DEFAULT_CAMERA);
	
	if( !camera )
	{
		printf("\nimagenet-camera:  failed to initialize video device\n");
		return 0;
	}
	
	printf("\nimagenet-camera:  successfully initialized video device\n");
	printf("    width:  %u\n", camera->GetWidth());
	printf("   height:  %u\n", camera->GetHeight());
	printf("    depth:  %u (bpp)\n\n", camera->GetPixelDepth());
	

	/*
	 * create imageNet
	 */
	imageNet* net = imageNet::Create(argc, argv);
	
	if( !net )
	{
		printf("imagenet-console:   failed to initialize imageNet\n");
		return 0;
	}


	/*
	 * create openGL window
	 */
	glDisplay* display = glDisplay::Create();
	glTexture* texture = NULL;
	
	if( !display ) {
		printf("\nimagenet-camera:  failed to create openGL display\n");
	}
	else
	{
		texture = glTexture::Create(camera->GetWidth(), camera->GetHeight(), GL_RGBA32F_ARB/*GL_RGBA8*/);

		if( !texture )
			printf("imagenet-camera:  failed to create openGL texture\n");
	}
	
	
	/*
	 * create font
	 */
	cudaFont* font = cudaFont::Create();
	

	/*
	 * start streaming
	 */
	if( !camera->Open() )
	{
		printf("\nimagenet-camera:  failed to open camera for streaming\n");
		return 0;
	}
	
	printf("\nimagenet-camera:  camera open for streaming\n");
	
	
	/*
	 * processing loop
	 */
	float confidence = 0.0f;
	
	while( !signal_recieved )
	{
		void* imgCPU  = NULL;
		void* imgCUDA = NULL;
		
		// get the latest frame
		if( !camera->Capture(&imgCPU, &imgCUDA, 1000) )
			printf("\nimagenet-camera:  failed to capture frame\n");
		//else
		//	printf("imagenet-camera:  recieved new frame  CPU=0x%p  GPU=0x%p\n", imgCPU, imgCUDA);
		
		// convert from YUV to RGBA
		void* imgRGBA = NULL;
		
		if( !camera->ConvertRGBA(imgCUDA, &imgRGBA) )
			printf("imagenet-camera:  failed to convert from NV12 to RGBA\n");

		// classify image
		const int img_class = net->Classify((float*)imgRGBA, camera->GetWidth(), camera->GetHeight(), &confidence);
		float por_class = confidence * 100.0f;
		std::string str_class(net->GetClassDesc(img_class));
		char feed;

		if( por_class <= 90)			// Significa que frente a la camara no hay algun animalito
		{
			str_class = "NO-Animalito";
			por_class = 100.0f;
			feed = 'X';
		}
		
		std::string fee_class("@") ;
		
		
		if(str_class == "Guepardo" || str_class == "Leon" || str_class == "Cocodrilo" || str_class == "Oso")
		{
			fee_class = "Carnivoro";
			feed = 'C';
		}

		if(str_class == "Elefante" || str_class == "Primate" || str_class == "Jirafa" || str_class == "Rinoceronte" || str_class == "Hipopotamo")
		{
			fee_class = "Herbivoro";
			feed = 'H';
		}

		if( img_class >= 0 )
		{
			printf("imagenet-camera:  %2.5f%% class #%i (%s) \n", por_class, img_class, str_class.c_str());	

			if( font != NULL )
			{
				char str[256];
				sprintf(str, "%05.2f%% %s - %s", por_class, str_class.c_str(), fee_class.c_str());
				
				switch(feed) 
				{
					case 'C' :
						system("pwd && cd /home/nvidia/Escritorio/jetson-inference/build/aarch64/bin/networks/digitstojetson/Led3Bash && ./gpioLedRed.sh");
						sleep(1);					
						printf("Se prendio el LED ROJO \n");
						break;
					case 'H' :
						system("pwd && cd /home/nvidia/Escritorio/jetson-inference/build/aarch64/bin/networks/digitstojetson/Led3Bash && ./gpioLedGreen.sh");
						sleep(1);
						printf("Se prendio el LED VERDE \n");
						break;
					case 'X':
						system("pwd && cd /home/nvidia/Escritorio/jetson-inference/build/aarch64/bin/networks/digitstojetson/Led3Bash && ./gpioLedBlue.sh");
						sleep(1);
						printf("Se prendio el LED AZUL \n");
						break;
				}

				font->RenderOverlay((float4*)imgRGBA, (float4*)imgRGBA, camera->GetWidth(), camera->GetHeight(),
								    str, 0, 0, make_float4(255.0f, 255.0f, 255.0f, 255.0f));
			}
			
			if( display != NULL )
			{
				char str[256];
				sprintf(str, "TensorRT build %x | %s | %s | %04.1f FPS", NV_GIE_VERSION, net->GetNetworkName(), net->HasFP16() ? "FP16" : "FP32", display->GetFPS());
				//sprintf(str, "TensorRT build %x | %s | %04.1f FPS | %05.2f%% %s", NV_GIE_VERSION, net->GetNetworkName(), display->GetFPS(), confidence * 100.0f, net->GetClassDesc(img_class));
				display->SetTitle(str);	
			}	
		}	


		// update display
		if( display != NULL )
		{
			display->UserEvents();
			display->BeginRender();

			if( texture != NULL )
			{
				// rescale image pixel intensities for display
				CUDA(cudaNormalizeRGBA((float4*)imgRGBA, make_float2(0.0f, 255.0f), 
								   (float4*)imgRGBA, make_float2(0.0f, 1.0f), 
		 						   camera->GetWidth(), camera->GetHeight()));

				// map from CUDA to openGL using GL interop
				void* tex_map = texture->MapCUDA();

				if( tex_map != NULL )
				{
					cudaMemcpy(tex_map, imgRGBA, texture->GetSize(), cudaMemcpyDeviceToDevice);
					texture->Unmap();
				}

				// draw the texture
				texture->Render(100,100);		
			}

			display->EndRender();
		}
	}
	
	printf("\nimagenet-camera:  un-initializing video device\n");
	
	
	/*
	 * shutdown the camera device
	 */
	if( camera != NULL )
	{
		delete camera;
		camera = NULL;
	}

	if( display != NULL )
	{
		delete display;
		display = NULL;
	}
	
	printf("imagenet-camera:  video device has been un-initialized.\n");
	printf("imagenet-camera:  this concludes the test of the video device.\n");
	return 0;
}

