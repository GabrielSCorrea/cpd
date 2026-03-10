#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>

#define OUTPUT 1

#define FIND(item_offset, num_of_subitens, subitem_offset) ((item_offset * num_of_subitens) + subitem_offset)

int process_input(char* filename, int *c, int *d, int *s, double **scores);
int sort_documents_omp(int num_cabs, int num_docs, int num_subs, int *docs_cabs, double *scores);

int main(int argc, char **argv){
	double exec_time_omp;

	//input file
	if(argc == 1){
		printf("Missing input file\n");
		return -1;
	}
	
	double *scores;
	int num_of_cabinets, num_of_documents, num_of_subjects;

	// will return a 1D array with all scores.
	process_input(argv[1], &num_of_cabinets, &num_of_documents, &num_of_subjects, &scores);
	
	//1D array of all scores
	//[D0S0, D0S1, D0S2, D1S0, D1S2...]
	//array with the cabinet of document equal to the index
	int *docs_cabs = malloc(num_of_documents * sizeof(int));

	exec_time_omp = -omp_get_wtime();
	sort_documents_omp(num_of_cabinets, num_of_documents, num_of_subjects, docs_cabs, scores);
	exec_time_omp += omp_get_wtime();
	
	fprintf(stderr, "%.3fs\n", exec_time_omp);

	if(OUTPUT){
		for(int i = 0; i < num_of_documents; i++){
			printf("%d\n", docs_cabs[i]);
		}
	}

	free(scores);
	free(docs_cabs);

	return 0;
}


int sort_documents_omp(int num_cabs, int num_docs, int num_subs, int *docs_cabs, double *docs_scores){
	double *cab_scores = calloc(num_cabs * num_subs, sizeof(double)); // subjects scores of each cabinet
	// documents distances to each cabinet
	//[D0C0, D0C1, D0C2, D1C0, D1C1 ...]
	double *doc_distances = calloc(num_docs * num_cabs, sizeof(double));
	//holds how many documents there is in cabinet with id correspond to index
	int *num_docs_in_cab = calloc(num_cabs, sizeof(int));
	int doc_change;

	#pragma omp parallel shared(doc_change)
	{
		double l1, l2, l3, l4 = 0;
	
	//initial round-robin; 
	#pragma omp for nowait
	for (int doc = 0; doc < num_docs; doc++){
		int cab_id = doc % num_cabs;
		docs_cabs[doc] = cab_id;
	}
		
	//change docs until no changes loop...
	do{
		double aux;

		#pragma omp barrier
			
		#pragma omp single
		doc_change = 0;
	
		aux = -omp_get_wtime();

		//calculate new scores
		#pragma omp for nowait 
		for(int doc = 0; doc < num_docs; doc++){
			for(int sub = 0; sub < num_subs; sub++){
				#pragma omp atomic
				cab_scores[FIND(docs_cabs[doc], num_subs, sub)] += docs_scores[FIND(doc, num_subs, sub)];
			}
			
			#pragma omp atomic
			num_docs_in_cab[docs_cabs[doc]] += 1;
		}
		aux += omp_get_wtime();
		l1 += aux;
		#pragma omp barrier
		
		aux = -omp_get_wtime();
		//finish calculating score with the division by the number of documents in each cabinets
		#pragma omp for nowait 
		for(int cab_score = 0; cab_score < num_cabs*num_subs; cab_score++){
			int cab_id = cab_score / num_subs;
			if(num_docs_in_cab[cab_id] == 0) {
				cab_scores[cab_score] = 0;
			}else{
				cab_scores[cab_score] /= num_docs_in_cab[cab_id];
			}
		}
		aux += omp_get_wtime();
		l2 += aux;
		#pragma omp barrier

		aux = -omp_get_wtime();
		//calculate distances
		#pragma omp for nowait 
		for(int doc = 0; doc < num_docs; doc++){
			for(int cab = 0; cab < num_cabs; cab++){
				double sum = 0;
				for(int sub = 0; sub < num_subs; sub++){
					double dif = docs_scores[FIND(doc, num_subs, sub)] - cab_scores[FIND(cab, num_subs, sub)];
					sum += dif*dif;
				}
				doc_distances[FIND(doc, num_cabs, cab)] = sum; 
			}
		}
		aux += omp_get_wtime();
		l3 += aux;
		#pragma omp barrier
		

		aux = -omp_get_wtime();
		//change documents based on distances
		#pragma omp for nowait 
		for (int doc = 0; doc < num_docs; doc++) {
    		double min_dist = doc_distances[doc * num_cabs + 0];
    		int closer_cab = 0;

    		for (int cab = 0; cab < num_cabs; cab++) {
        		double dist = doc_distances[doc * num_cabs + cab];
        		if (dist < min_dist) {
            		min_dist = dist;
            		closer_cab = cab;
        		}
    		}
				
			if(docs_cabs[doc] != closer_cab){
				#pragma omp critical
   			 	doc_change = 1;
				docs_cabs[doc] = closer_cab;
			}
		}
		aux += omp_get_wtime();
		l4 += aux;

		#pragma omp single
		{
			memset(cab_scores, 0, num_cabs * num_subs * sizeof(double));
			memset(num_docs_in_cab, 0, num_cabs * sizeof(int));
		}

	}while(doc_change);

	fprintf(stderr, "Loop 1: %.3fs - t%d \n ", l1, omp_get_thread_num());
    #pragma omp barrier
	fprintf(stderr, "Loop 2: %.3fs - t%d \n ", l2, omp_get_thread_num());
    #pragma omp barrier
	fprintf(stderr, "Loop 3: %.3fs - t%d \n ", l3, omp_get_thread_num());
    #pragma omp barrier
	fprintf(stderr, "Loop 4: %.3fs - t%d \n ", l4, omp_get_thread_num());

	} // end parallel region
	
	
	free(cab_scores);
	free(doc_distances);
	free(num_docs_in_cab);

	return 0;
}

int process_input(char* filename, int* num_of_cabinets, int* num_of_documents, int* num_of_subjects, double** scores){
    FILE *file_ptr = fopen(filename, "r");
    if(file_ptr == NULL){
        fprintf(stderr, "Error open input file\n");
        return -1;
    }

    char reader[100];
    fgets(reader, 100, file_ptr);
    sscanf(reader, "%d %d %d", num_of_cabinets, num_of_documents, num_of_subjects);

    *scores = malloc((*num_of_documents) * (*num_of_subjects) * sizeof(double));

    long current_pos = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_END);
    long file_size = ftell(file_ptr);
    int bytes_per_line = (file_size - current_pos) / *num_of_documents;
    fseek(file_ptr, current_pos, SEEK_SET);

    char *buffer = malloc((bytes_per_line + 2) * sizeof(char));

    for(int line = 0; line < *num_of_documents; line++){
        int id = -1;
        int subjects = 0;

        fgets(buffer, bytes_per_line + 2, file_ptr);
        char *values = strtok(buffer, " ");

        while(values != NULL && strcmp("\n", values) != 0){
            if(id != -1){
                sscanf(values, "%lf", &(*scores)[FIND(id, *num_of_subjects, subjects)]);
                subjects++;
            } else {
                sscanf(values, "%d", &id);
            }
            values = strtok(NULL, " ");
        }
    }

    free(buffer);
    fclose(file_ptr); 
    return 0;
}

