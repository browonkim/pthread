
# Pthread를 사용한 Producer 와 Consumer 프로그래밍 

Linux의 Pthread를 사용하여 Producer/Consumer 작동을 응용하여 
자동차 공장의 불량 부품 처리과정을 시뮬레이션 하는 프로젝트입니다.  

부품 처리 과정은 N 개의 stageThread 가 담당하고 처음과 마지막은 startThread와 endThread 가 담당합니다. 
부품은 autoPart 구조체로 표현되고 부품의 고유번호(partNumber)를 갖습니다. 
  
 아래 그림과 같이 startThread 와 endThread 사이에는 N 개의 stageThread가 존재하고 각각의 Thread 사이에는 autoPartBox 가 있어 
 autoPart를 주고받는 데 사용합니다. autoPartBox는 크기가 정해져 있어 해당 크기만큼의 부품을 받으면 더 이상 받을 수가 없습니다.  
  
 ![Structure_diagram](./structure_diagram.jpg)
 각 thread의 역할은 다음과 같습니다.

## 1) startThread
부품(autoPart)을 만들어 내어 첫 번째 stageThread 에 전달합니다. autoPart를 할당받아 첫 번째 stageThread와 사이에 있는 autoPartBox에 보냅니다. 각 autoPart
의 부품 번호는 rand()의 함수를 사용하여 절댓값으로 정합니다. 이 때 시드(seed) 값은 ‘100’ 으로 설정합니다. 모든 부품을 다 만들어 보낸 뒤에는 ENDMARK(=-1)를 부품 번호로 갖는 autoPart를 전달합
니다. 만들어야 하는 부품의 수는 인자로 받고 모든 부품 번호의 합을 pthread_exit의 status 값으로 전달합니다. 
## 2) stageThread
N 개의 stageThread는 startThread와 endThread 사이에 존재하며 위 그림과
같이 각각의 Thread들 사이에는 autoPartBox가 있습니다. stageThread는 앞쪽
autoPartBox에서 autoPart를 받아 뒤쪽 autoPartBox 로 보냅니다. 이 과정에서
autoPart의 번호가 defectNumber 의 배수이면 해당 autoPart 는 전달하지 
않습니다. defectNumber의 배수인 autoPart의 partNumber 는 모두 합하여
pthread_exit의 status 값으로 전달합니다. ENDMARK 의 autoPart는 그대로 전달합니다. defectNumber 와 stage ID 값은
인자로 받습니다. stage ID 는 그림에서 보듯이 1부터 시작합니다. autoPartBox의
ID는 0부터 시작합니다. 
## 3) endThread
마지막 autoPartBox에서 autoPart를 받습니다. 이 autoPart는 모든
stageThread에서 defectNumber의 배수 테스트를 모두 통과하고
도달한 것입니다. 인자로는 Stage 번호를 마지막 stageThread (= N+1) 라 가정하고
전달받습니다. ENDMARK autoPart를 받으면 그동안 전달받은 autoPart의
partNumber를 모두 합하여 pthread_exit의 status 값으로 전달합니다. 
## 4) main()
명령어 인자로 NSTAGES BOXSIZE NPART defect_numbers 를 받습니다. 모든
인자의 값은 제한이 없습니다.  
*NSTAGES* – stageThread의 수.  
*BOXSIZE* – autoPartBox의 크기.  
따라서 autoPartBox에서는 autoPart를 linked list의 fifo queue로 구현 합니다.  
*NPART* – 생성되는 autoPart의 수  
*defect_numbers* – NSTAGES 개수 만큼 1 번 stageThread부터 시작하여 차례로
사용하게 될 defectNumber 의 리스트  
