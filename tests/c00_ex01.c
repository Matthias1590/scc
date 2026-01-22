/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   c00_ex01.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mawijnsm <mawijnsm@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 11:37:07 by mawijnsm          #+#    #+#             */
/*   Updated: 2026/01/22 18:54:43 by mawijnsm         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int write(int fd, char *c, int count);

void	ft_print_alphabet()
{
	char	c;

	c = 'a';
	while (c <= 'z')
	{
		write(1, &c, 1);
		c++;
	}
}

int main()
{
    ft_print_alphabet();
    write(1, "\n", 1);

    return 0;
}
