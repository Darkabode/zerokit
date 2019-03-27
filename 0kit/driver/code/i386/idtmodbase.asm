.586
.MODEL FLAT, STDCALL
OPTION CASEMAP:NONE

.code

; Алгоритм:
; 1. Берём очередной элемент в IDT таблице и проверяем присутсвие бита Present, и соответствие GateType ~ Interrupt | Tarp
; 2. Вычисляет адрес ISR
; 3. Постранично спускаемся вниз в поиска сигнатуры MZ, ограничивая спуск до нижнего предела системной памяти (80000000h)
; 4. При обнаружении сигнатуры MZ, проверяем наличие сигнатуры PE
; 5. При обнаружении сигнатуры PE, возвращаем адрес базы

GetModuleBaseFromIDTEntry PROC C index:DWORD
	local idtr:FWORD
  
	cli
	sidt idtr                        ; Сохраняем содержимое регистра IDTR
	sti
   
	movzx ecx, word ptr idtr         ; Считываем лимит IDT
	shr ecx, 3                       ; Получаем количество элементов таблицы
	dec ecx
	
	cmp ecx, index			         ; Проверяем принадлежность IDT-таблице
	jbe base_not_found                ; За пределами -> покидаем функцию

	mov ebx, index
	shl ebx, 3
	add ebx, dword ptr idtr + 2     ; Вычисляем адрес очередного IDT-элемента
	
	cmp byte ptr [ebx + 5], 80h      ; Проверяем бит P (бит присутствия)
	jz base_not_found                ; Пусто -> переходим к следующему IDT-элементу
	mov al, byte ptr [ebx + 5]       ; Ложим в al маску GateType и проверяем наличие первого и второго битов (Interrupt Gate, Trap Gate)
	and al, 06h
	cmp al, 06h
	jne base_not_found               ; Не подходит -> переходим к следующему IDT-элементу
   
	movzx edx, word ptr [ebx + 6]
	shl edx, 16
	mov dx, word ptr [ebx]
	and dx, 0F000h
	mov ebx, edx

down_page:
	cmp edx, 80000000h
	jbe base_not_found
	cmp word ptr [edx], 5A4Dh        ; Проверяем сигнатуру 'MZ'
	jne jump_down_to_page
	sub eax, eax
	mov eax, dword ptr [edx + 3Ch]
	cmp word ptr [edx + eax], 4550h  ; Проверяем сигнатуру 'PE'
	jz base_found
jump_down_to_page:
	sub edx, 1000h
	jmp down_page
   
base_not_found:
   xor edx, edx

base_found:
   mov eax, edx
   ret
GetModuleBaseFromIDTEntry ENDP

END