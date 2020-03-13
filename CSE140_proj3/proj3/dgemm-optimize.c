#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

 void dgemm( int m, int n, float *A, float *C )
 {
// 	//padding matrix
int even_n = n;
int even_m = m;
int count =0;
bool m_odd = false;
bool n_odd = false;

// printf("m %d",m);
// printf("n %d", n);
if(m%2 == 1){
	//printf("1\n");
	//make n even
	even_m = m+1;
	count++;
	m_odd =true;
}
else if(n%2==1){
	//printf("2\n");
	//make m even //printf //printf("1");("1");
	even_n = n+1;
	count++;
	n_odd =true;
}
else {
	//both m and n are even
	// printf("1\n");
	  for( int j = 0; j < m; j++ )
   		 for( int k = 0; k < n; k++ ) 
      		for( int i = 0; i < m; i++ ) 
				C[i+j*m] += A[i+k*m] * A[j+k*m];
}

 //printf("1");
//creating double matrix
//float** pad_Matrix =(float**)  malloc (m * sizeof(float*));
 //printf("2");
// for(int i = 0; i < m; i++){
// 	pad_Matrix[i] = (float*) malloc(n*sizeof(float));
// }
if(count>0){
float* pad_Matrix = (float*) malloc (even_m * even_n *sizeof(float));
//float* pad_Matrix_n = (float*) malloc (even_n * even_m *sizeof(float));

for(int i = 0; i< (even_m*even_n); i++){   		 
			pad_Matrix[i] = 0;
			//C[i + j * m] += pad_Matrix[i][j];
}
if(m_odd == true){
	for(int i = 0; i < (m*n); i++){
	//for(int j = 0; j)
	
		if(m_odd ==true){
			pad_Matrix[i] = A[i];
		}   


			
			//C[i + j * m] += pad_Matrix[i][j];
	}

}




else if(n_odd == true){
	//int i = 0;
int j = 0;
//int k =0;
	for(int i = 0; i < (m*n); i++){
		if(i % n == 0){
			j=j++;
			pad_Matrix[j] = A[i];
			j++;
			//i--;
		}
		else {
			pad_Matrix[j] = A[i];
			j++;
		}

	
		//if(n_odd ==true && i != 0 && n%i != 0){
			
		//}   
		// else if(i == 0){
		// 	pad_Matrix[i] = A[i];
		// }


			
			//C[i + j * m] += pad_Matrix[i][j];
	}
}



 //printf("3");
//matrix multiplication
//if(n_odd == true){
for(int j = 0; j< m; j++){   		 
	for( int k = 0; k < even_n; k++ ){ 
		for (int i = 0; i< m; i++){
			C[i+j*m] += pad_Matrix[i+k*m] * pad_Matrix[j+k*m];
			//C[i + j * m] += pad_Matrix[i][j];
		}
	}
//}

// if(m_odd == true){
// for(int i = 0; i< even_m; i++){   		 
// 	for( int k = 0; k < n; k++ ){ 
// 		for (int j = 0; j< even_m; j++){
// 			C[i+j*m] += pad_Matrix[i+k*m] * pad_Matrix[j+k*m];
// 			//C[i + j * m] += pad_Matrix[i][j];
// 		}
// 	}
// }
// }

}
}  
}




    //Unrolling

    // register float a = 0.0;
    
    // for(int i = 0; i < m; i++){
    
    //     for(int j = 0; j < n; j++){
            
    //         a = A[i + j * m];
            
    //         for(int k = 0; k < m; k++){
            
	//             C[i + k * m] = a * A[k + j * m] + C[i + k * m];
            
    //         }
        
    //     }
        
    //  }






// void dgemm( int m, int n, float *A, float *C )
// {
// 	//prefetching
// 	//bool hi;
// 	//int half = m/2;

// 	for( int i = 0; i < m; i++ ){
// 		for( int k = 0; k < n; k++ ) {
// 			//for( int j = 0; j < m; j=j+1 ) {
// 				for( int j = 0; j < m; j=j+2 ) {
// 					//int t =0;
// 					//float x = A[i+k*m];
// 					//float y = A[j+k*m];
// 					// for(t = j+1; t<=half; t=t+2){
// 					 	C[i+j*m] += A[i+k*m] * A[j+k*m];
// 					 	//C[i+j*m] += x * y;
// 					// }
// 					if(j+1 <= m-1){
// 						C[i+(j+1)*m] += A[i+k*m] * A[(j+1)+k*m];
// 						//printf("2\n");
// 						}
// 					// else if(i%2== 1 && hi ){
// 					// 	C[(i+1)+j*m] += A[(i+1)+k*m] * A[j+k*m];
// 					// 	//printf("3");
// 					// 	hi = !hi;
// 					// 	}
// 			}
// 		}
// 		 //i = !hi;
// 		 //i++;
// 	}
//  }
    		
